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
HTTPClient httpGraphite;

// Sensors
DHT dht(DHT_PIN, DHT_TYPE);
UltraSonicDistanceSensor distanceSensor(ULTRASONIC_PIN_TRIG, ULTRASONIC_PIN_ECHO);

// LED
LedControl lc=LedControl(LED_DIN,LED_CLK,LED_CS,0);

// LED visualisations
byte smile[8] = {0x3C, 0x42, 0xA5, 0x81, 0xA5, 0x99, 0x42, 0x3C};
byte neutral[8] = {0x3C, 0x42, 0xA5, 0x81, 0xBD, 0x81, 0x42, 0x3C};
byte sad[8] = {0x3C, 0x42, 0xA5, 0x81, 0x99, 0xA5, 0x42, 0x3C};
byte off[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
byte err[8] = {0x00, 0x00, 0x78, 0x40, 0x70, 0x40, 0x78, 0x00};

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
void submitToInflux(unsigned long ts, float cels, float hum, float hic, int moist, long height, float light)
{
  String influxClient = String("https://") + INFLUX_HOST + "/api/v2/write?org=" + INFLUX_ORG_ID + "&bucket=" + INFLUX_BUCKET + "&precision=s";
  String body = String("temperature value=") + cels + " " + ts + "\n" + "humidity value=" + hum + " " + ts + "\n" + "index value=" + hic + " " + ts + "\n" + "moisture value=" + moist + " " + ts + "\n" + "light value=" + light + " " + ts + "\n" + "height value=" + height + " " + ts;

  // Submit POST request via HTTP
  httpInflux.begin(influxClient);
  httpInflux.addHeader("Authorization", INFLUX_TOKEN);
  int httpCode = httpInflux.POST(body);
  Serial.printf("Influx [HTTPS] POST...  Code: %d\n", httpCode);
  httpInflux.end();
}

// Function to submit logs to Loki
void submitToLoki(unsigned long ts, float cels, float hum, float hic, int moist, long height, float light, String message)
{
  String lokiClient = String("https://") + LOKI_USER + ":" + LOKI_API_KEY + "@logs-prod-us-central1.grafana.net/loki/api/v1/push";
  String body = String("{\"streams\": [{ \"stream\": { \"plant_id\": \"2020_12_17\", \"monitoring_type\": \"avocado\"}, \"values\": [ [ \"") + ts + "000000000\", \"" + "temperature=" + cels + " humidity=" + hum + " heat_index=" + hic + " soil_moisture=" + moist + " height=" + height + " light=" + light + " status=" + message + "\" ] ] }]}";

  // Submit POST request via HTTP
  httpLoki.begin(lokiClient);
  httpLoki.addHeader("Content-Type", "application/json");
  int httpCode = httpLoki.POST(body);
  Serial.printf("Loki [HTTPS] POST...  Code: %\n", httpCode);
  httpLoki.end();
}

// Function to submit logs to Graphite
void submitToGraphite(unsigned long ts, float cels, float hum, float hic, int moist, long height, float light) {
  // build hosted metrics json payload
  String body = String("[") +
    "{\"name\":\"temperature\",\"interval\":" + INTERVAL + ",\"value\":" + cels + ",\"mtype\":\"gauge\",\"time\":" + ts + "}," +
    "{\"name\":\"humidity\",\"interval\":" + INTERVAL + ",\"value\":" + hum + ",\"mtype\":\"gauge\",\"time\":" + ts + "}," +
    "{\"name\":\"heat_index\",\"interval\":" + INTERVAL + ",\"value\":" + hic + ",\"mtype\":\"gauge\",\"time\":" + ts + "}," +
    "{\"name\":\"moisture\",\"interval\":" + INTERVAL + ",\"value\":" + moist + ",\"mtype\":\"gauge\",\"time\":" + ts + "}," +
    "{\"name\":\"height\",\"interval\":" + INTERVAL + ",\"value\":" + height + ",\"mtype\":\"gauge\",\"time\":" + ts + "}," +
    "{\"name\":\"light\",\"interval\":" + INTERVAL + ",\"value\":" + light + ",\"mtype\":\"gauge\",\"time\":" + ts + "}]";

  // submit POST request via HTTP
  httpGraphite.begin("https://graphite-us-central1.grafana.net/metrics");
  httpGraphite.setAuthorization(GRAPHITE_USER, GRAPHITE_API_KEY);
  httpGraphite.addHeader("Content-Type", "application/json");

  int httpCode = httpGraphite.POST(body);
  Serial.printf("Graphite [HTTPS] POST...  Code: %\n", httpCode);
  httpGraphite.end();
}

// Function to display state of the plant on LED Matrix
void displayState(byte character []) {
  unsigned long currHour = ntpClient.getHours();
  bool shouldDisplay = (currHour < 21 && currHour > 8);   // Turn off in the night
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

// Function to read soil moisture measurement
int getSoilMoisture() {
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

// Function to read height of the plant
double getHeight() {
  double manualCurrentHeight = 25;
  double height = DISTANCE_FROM_POT - distanceSensor.measureDistanceCm();
    // Serial.println(height);
  if (height > manualCurrentHeight - 3 && height < manualCurrentHeight + 4) {
    return height;
  }

  return manualCurrentHeight;  
}

// Function to read light condition
float getLightLux() {
  float sensor_value = analogRead(TEMP6000_PIN);  // Get raw sensor reading
  float volts = sensor_value * TEMP6000_VCC / 1024.0;  // Convert reading to voltage
  float amps = volts / 10000.0; // Convert to amps across 10K resistor
  float microamps = amps * 1000000.0; // Convert amps to microamps
  float lux = microamps * 2.0; 
  
  return lux;
}

// Function to check if any reading failed
bool checkIfReadingFailed(float hum, float cels, int moist, double height, float light) {
  if (isnan(hum) || isnan(cels) || isnan(moist) || isnan(height) || isnan(light)) {
      // Print letter E as error
      displayState(err);
      Serial.println(F("Failed to read from some sensor!"));
      return true;
    }
    return false;
}

// Function to create message and display current state of the plant
String createAndDisplayState (int moist, float cels) {
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

    return currentState;
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
  int moist = getSoilMoisture();

  // Read height of plant
  double height = getHeight();

  //Read the light in lux
  float light = getLightLux();

  // Check if any reads failed and return early
  if (checkIfReadingFailed(hum, cels, moist, height, light)) {
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(cels, hum, false);

  // Convert data to state of the plant
  String message = createAndDisplayState(moist, cels);

  // Submit data
  submitToInflux(ts, cels, hum, hic, moist, height, light);
  submitToGraphite(ts, cels, hum, hic, moist, height, light);
  submitToLoki(ts, cels, hum, hic, moist, height, light, message);

  // wait INTERVAL seconds, then do it again
  delay(INTERVAL * 1000);
}
