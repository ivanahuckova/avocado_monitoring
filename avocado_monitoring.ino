#include <Arduino.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <HCSR04.h>
#include <LedControl.h>
#include "config.h" // Configuration file

// NTP Client
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP);

// Loki & Infllux Clients
HTTPClient httpInflux;
HTTPClient httpLoki;

// Sensors
DHT dht(DHT_PIN, DHT_TYPE);
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);

// LED
LedControl lc=LedControl(LED_DIN,LED_CLK,LED_CS,0);

// Facial Expression for LED
byte smile[8] = {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C};
byte neutral[8] = {0x3C, 0x42, 0xA5, 0x81, 0xBD, 0x81, 0x42, 0x3C};
byte sad[8] = {0x3C, 0x42, 0xA5, 0x81, 0x99, 0xA5, 0x42, 0x3C};
byte off[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte err[8] = {0x00, 0x00, 0x78, 0x40, 0x70, 0x40, 0x78, 0x00}

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
}

// Function to submit metrics to Influx
void submitToInflux(unsigned long ts, float cels, float hum, float hic, int moist, long height)
{
  String influxClient = String("https://") + INFLUX_HOST + "/api/v2/write?org=" + INFLUX_ORG_ID + "&bucket=" + INFLUX_BUCKET + "&precision=s";
  String body = String("temperature value=") + cels + " " + ts + "\n" + "humidity value=" + hum + " " + ts + "\n" + "index value=" + hic + " " + ts + "\n" + "moisture value=" + moist + " " + ts + "\n" + "height value=" + height + " " + ts;

  // Submit POST request via HTTP
  httpInflux.begin(influxClient);
  httpInflux.addHeader("Authorization", INFLUX_TOKEN);
  int httpCode = httpInflux.POST(body);
  Serial.printf("Influx [HTTPS] POST...  Code: %d", httpCode);
  httpInflux.end();
}

// Function to submit logs to Loki
void submitToLoki(unsigned long ts, float cels, float hum, float hic, int moist, long height, String message)
{
  String lokiClient = String("https://") + LOKI_USER + ":" + LOKI_API_KEY + "@logs-prod-us-central1.grafana.net/loki/api/v1/push";
  String body = String("{\"streams\": [{ \"stream\": { \"monitoring\": \"avocado_monitoring\", \"monitoring_type\": \"avocado\"}, \"values\": [ [ \"") + ts + "000000000\", \"" + "temperature=" + cels + " humidity=" + hum + " heat_index=" + hic + " soil_moisture=" + moist + " height=" + height + " status=" + message + "\" ] ] }]}"

  // Submit POST request via HTTP
  httpLoki.begin(lokiClient);
  httpLoki.addHeader("Content-Type", "application/json");
  int httpCode = httpLoki.POST(body);
  Serial.printf("Influx [HTTPS] POST...  Code: %d", httpCode);
  httpLoki.end();
}

// Function to display state of the plant on LED Matrix
void displayState(byte character []) {
  unsigned long hours = ntpClient.getHours();
  bool shouldDisplay = (hours < 19 && hours > 8);   // Turn off in the night
  if (shouldDisplay)  {
    for (int i = 0; i < 8; i++) {
      lc.setRow(0, i, character[i]);
    }
  } else {
    for (int i = 0; i < 8; i++)  {
      lc.setRow(0, i, off[i]);
    }
  }
}

// Function to create message and display current state of the plant
String convertDataToState (int moist, float cels) {
    String currentState = "";
    if (moist) {
      currentState = "DRY critical";
      displayState(sad);
    } else if( cels < 16 ) {
      currentState = "COLD warning";
      displayState(neutral);
    } else if( cels > 26 ) {
      currentState = "HOT warning";
      displayState(neutral);
    } else {
      currentState = "OK info";
      displayState(smile);
    }

    return currentState
}

// Function to read soil moisture measurement
int readSoilMoistureSensor() {
  // Turn the sensor ON
	digitalWrite(MOISTURE_POWER, HIGH);  
  // Allow power to settle
	delay(10);        
  // Read the digital value form sensor                   
	int val = digitalRead(MOISTURE_PIN); 
  // Turn the sensor OFF
	digitalWrite(MOISTURE_POWER, LOW);   
  // Return moisture value
	return val;                          
}

// Function to check if any reading failed
checkIfReadingFailed(float hum, floatt cels, int moist, double height) {
  if (isnan(hum) || isnan(cels) || isnan(moist) || isnan(height)) {
      // Print letter E as error
      displayState(err);
      Serial.println(F("Failed to read from some sensor!"));
      return;
    }
}

// ========== MAIN FUNCTIONS: SETUP & LOOP ========== 
// SETUP: Function called at boot to initialize the system
void setup() {

  // Set up soil moisture pins - initially keep the moisture sensor OFF (prevents sensor rusting)
  pinMode(MOISTURE_POWER, OUTPUT);
	digitalWrite(MOISTURE_POWER, LOW);

  // Start the serial output at 115,200 baud 
  Serial.begin(115200);

  // Connect to WiFi
  setupWiFi();

  // Initialize a NTPClient to get time
  ntpClient.begin();

  // Start the DHT sensor
  dht.begin();

  // Start LED Matrix
  lc.shutdown(0,false);       
  lc.setIntensity(0,0.0000001);      //Adjust the brightness maximum is 15
  lc.clearDisplay(0);    
}

// LOOP: Function called in a loop to read from sensors and send them do databases
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

  // Read temperature as Celsius (the default)
  float cels = dht.readTemperature();

  // Read soil moisture (DRY: 1, WET: 0)
  int moist = readSoilMoistureSensor();

  // Read height of plant
  double distanceFromGround = 45.40;
  double height = distanceFromGround - distanceSensor.measureDistanceCm();

  // Check if any reads failed and exit early (to try again)
  checkIfReadingFailed(hum, cels, moist, height)

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(cels, hum, false);

  // Convert data to state of the plant
  convertDataToState(moist, cels);

  // Submit data
  submitToInflux(ts, cels, hum, hic, moist, height);
  submitToLoki(ts, cels, hum, hic, moist, height, message);

  // wait INTERVAL seconds, then do it again
  delay(INTERVAL * 1000);
}
