#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>

#include <HCSR04.h>

#include "config.h"

// NTP Client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

// Ultrasonic Sensor
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);  // Initialize sensor that uses digital pins 4 and 5

// Influx Client
HTTPClient httpInflux;
HTTPClient httpLoki;

// Function to set up the connection to the WiFi AP
void setupWiFi() {
  Serial.print("Connecting to '");
  Serial.print(WIFI_SSID);
  Serial.print("' ...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("connected");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  randomSeed(micros());
}

// Function to submit metrics to locally run Influx
void submitHostedInflux(unsigned long ts, long dist) {

  double height = 45.40 - dist;
  
  // Build body
  String body = String("height value=") + height + " " + ts;

  // Submit POST request via HTTPS
  httpInflux.begin(String("https://") + INFLUX_HOST + "/api/v2/write?org=" + INFLUX_ORG_ID + "&bucket=" + INFLUX_BUCKET + "&precision=s", ROOT_CA);
  httpInflux.addHeader("Authorization", INFLUX_TOKEN);

  int httpCode = httpInflux.POST(body);
  if (httpCode > 0) {
    Serial.printf("Influx [HTTPS] POST...  Code: %d  Response: ", httpCode);
   Serial.println();
  } else {
    Serial.printf("Influx [HTTPS] POST... Error: %s\n", httpInflux.errorToString(httpCode).c_str());
  }

  httpInflux.end();
}

void submitLogsToLoki(unsigned long ts, long dist) {
  double height = 45.40 - dist;

  String logMessage = String("height=") + height +" info";
  String logJson = String("{\"streams\": [{ \"stream\": { \"monitoring\": \"avocado_monitoring\", \"monitoring_type\": \"height\"}, \"values\": [ [ \"") + ts + "000000000\", \"" + logMessage + "\" ] ] }]}";   
  Serial.println(logJson);

  // Submit POST request via HTTPS
  httpLoki.begin(String("https://") + LOKI_USER + ":" + LOKI_API_KEY + "@logs-prod-us-central1.grafana.net/loki/api/v1/push");
  httpLoki.addHeader("Content-Type", "application/json");

  int httpCode = httpLoki.POST(logJson);
  if (httpCode > 0) {
    Serial.printf("Loki [HTTPS] POST...  Code: %d  Response: ", httpCode);
    Serial.println();
  } else {
    Serial.printf("Loki [HTTPS] POST... Error: %s\n", httpLoki.errorToString(httpCode).c_str());
  }

  httpLoki.end();
}


//  Function called at boot to initialize the system
void setup() {
  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  // Connect to WiFi
  setupWiFi();

  // Initialize a NTPClient to get time
  ntpClient.begin();
}


// Function called in a loop to read temp/humidity and submit them to hosted metrics
void loop() {
  // Reconnect to WiFi if required
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    yield();
    setupWiFi();
  }

  // Update time via NTP if required
  while(!ntpClient.update()) {
    yield();
    ntpClient.forceUpdate();
  }

  // Get current timestamp
  unsigned long ts = ntpClient.getEpochTime();

  double dist = distanceSensor.measureDistanceCm();


  // Check if any reads failed and exit early (to try again).
  if (isnan(dist)) {
    Serial.println(F("Failed to read from sensor!"));
    return;
  }

  Serial.println(dist);

  yield();
  submitHostedInflux(ts, dist);
  submitLogsToLoki(ts, dist);
  

  // wait INTERVAL second, then do it again
  delay(INTERVAL * 1000);
}
