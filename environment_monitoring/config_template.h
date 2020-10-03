// WiFi
#define WIFI_SSID     "your_wifi_name" // Add wifi name
#define WIFI_PASSWORD "your_wifi_password" // Add wifi passowrd

#define ID "avocadoEnv" // Add unique name for this sensor
#define INTERVAL 60  // Add interval (e.g. 1 min)

#define DHT_PIN 27    // Which pin is DHT 11 connected to
#define DHT_TYPE DHT11 // Type DHT 11

#define MOISTURE_POWER 17 // Which pin is soil moisture sensor connected to as a source of power (prevents fast corrosioon)
#define MOISTURE_PIN 16 // Which pin is soil moisture sensor connected to

#define RED_PIN 5 // Which pin is LED module connected to
#define GREEN_PIN 13 // Which pin is LED module connected to
#define BLUE_PIN 12 // Which pin is LED moduleconnected to

#define INFLUX_HOST "your_influx_host" // Influx host (e.g. eu-central-1-1.aws.cloud2.influxdata.com)
#define INFLUX_ORG_ID "your_influx_org_id" // Org id
#define INFLUX_TOKEN "Token your_token" // Influx token
#define INFLUX_BUCKET "your_sourdough_bucket" // Influx bucket that we set up for this host


// How to get the Root CA cert:
// https://techtutorialsx.com/2017/11/18/esp32-arduino-https-get-request/

#define ROOT_CA "-----BEGIN CERTIFICATE-----" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexampleexampleexampleexampleexampleexampleexampleexamplee" \
"exampleexample" \
"-----END CERTIFICATE-----"