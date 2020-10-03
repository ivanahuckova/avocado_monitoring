#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <HCSR04.h>
#include <analogWrite.h>

#include "config.h"

// NTP Client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

// DHT Sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Influx Client
HTTPClient httpInflux;

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

// Function to submit metrics to Influx
void submitHostedInflux(unsigned long ts, float cels, float hum, float hic, int moist) {

  // Build body
  String body = String("temperature value=") + cels + " " + ts + "\n" +
                "humidity value=" + hum + " " + ts + "\n" +
                "index value=" + hic + " " + ts + "\n" +
                "moisture value=" + moist + " " + ts + "\n";

  // Submit POST request via HTTP
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

// Function to return the analog soil moisture measurement
int readSoilMoistureSensor() {
  // Turn the sensor ON
	digitalWrite(MOISTURE_POWER, HIGH);  
  // Allow power to settle
	delay(10);        
  // Read the analog value form sensor                   
	int val = digitalRead(MOISTURE_PIN); 
  // Turn the sensor OFF
	digitalWrite(MOISTURE_POWER, LOW);   
  // Return analog moisture value
	return val;                          
}

// Update color of LED sensor
void updateLedColor(int red_value, int green_value, int blue_value) {
    analogWrite(RED_PIN, red_value);
    analogWrite(GREEN_PIN, green_value);
    analogWrite(BLUE_PIN, blue_value);
}

// Function called at boot to initialize the system
void setup() {
  // Set up moisture pins - initially keep the moisture sensor OFF
  pinMode(MOISTURE_POWER, OUTPUT);
	digitalWrite(MOISTURE_POWER, LOW);

  // Set up RGB led pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Start the serial output at 115,200 baud
  Serial.begin(115200);

  // Connect to WiFi
  setupWiFi();

  // Initialize a NTPClient to get time
  ntpClient.begin();

  // Start the DHT sensor
  dht.begin();

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
  while (!ntpClient.update()) {
    yield();
    ntpClient.forceUpdate();
  }

  // Get current timestamp
  unsigned long ts = ntpClient.getEpochTime();

  // Read humidity
  float hum = dht.readHumidity();
  yield();

  // Read temperature as Celsius (the default)
  float cels = dht.readTemperature();
  yield();

  // Read soil moisture (DRY: 1, WET: 0)
  int moist = readSoilMoistureSensor();

  // Check if any reads failed and exit early (to try again)
  if (isnan(hum) || isnan(cels) || isnan(moist)) {
    // White color
    updateLedColor(255, 255, 255);
    Serial.println(F("Failed to read from some sensor!"));
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(cels, hum, false);

    if(moist) {
      Serial.println("Avocado needs water");
      // Red color
      updateLedColor(255, 0, 0);
    } else if( cels < 16 ) {
      Serial.println("Avocado is cold");
      // Blue color 
      updateLedColor(0, 0, 255);
    } else if( cels > 26 ) {
      // Yellow color
      updateLedColor(255, 255, 0);
      Serial.println("Avocado is hot");
    } else {
      // Green color
      updateLedColor(0, 255, 0);
      Serial.println("Avocado is doing fine");
    }

  yield();
  submitHostedInflux(ts, cels, hum, hic, moist);

  // wait INTERVAL s, then do it again
  delay(INTERVAL * 1000);
}
