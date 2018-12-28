#include <ESP8266WiFi.h>            // https://arduino-esp8266.readthedocs.io/en/latest/index.html
#include <ESP8266HTTPClient.h>
#include <SimpleDHT.h>
#include <Crypto.h>                 // https://github.com/intrbiz/arduino-crypto // this is esp2866 specific!
#include <base64.h>

#define WIFI_SSID "this_should_be_the_wifi_ssid"
#define WIFI_PASS "this_should_be_your_wifi_password"
#define SENSOR_DATA_POST_URL "http://192.168.1.10:5000/station-data"
#define STATION_ID "limonaia"

#define HMAC_KEY_LENGTH 46
static uint8_t hmac_key[HMAC_KEY_LENGTH] = {49, 50, 97, 115, 112, 111, 100, 117, 52, 104, 114, 106, 49, 195, 168, 50, 51, 57, 48, 106, 114, 49, 195, 168, 48, 101, 57, 102, 106, 49, 195, 168, 48, 101, 105, 110, 102, 49, 195, 168, 50, 48, 101, 43, 51, 49};

#define CONNECTION_RETRY_INTERVAL 500
#define SENSOR_DATA_ACQUISITION_SUCCESS 0
#define DHT_PIN 2
static SimpleDHT11 dht(DHT_PIN);

const int sample_interval = 3000;

void setup(){
  Serial.begin(115200);
  Serial.println();
  delay(5000);
  Serial.println("Initializing WIFI...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Done");
  Serial.print("Connecting");
  wait_until_connected(CONNECTION_RETRY_INTERVAL);
  Serial.println();
}

void loop() {
  Serial.println("+loop+");
  int start_time = millis();
  wait_until_connected(CONNECTION_RETRY_INTERVAL);
  float temperature = 0;
  float humidity = 0;
  if (acquire_sensor_data(&temperature, &humidity) == SENSOR_DATA_ACQUISITION_SUCCESS) {
      post_station_data(SENSOR_DATA_POST_URL, STATION_ID, temperature, humidity);
  }
  int elapsed = millis() - start_time;
  Serial.println("-loop-");

  delay(max(sample_interval - elapsed, 0));
}

void wait_until_connected(int retry_interval) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(retry_interval);
    Serial.print(".");
  }
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

int acquire_sensor_data(float *temperature, float *humidity) {
  // start working...
  Serial.println("Sampling DHT11...");
  
  // read without samples.
  byte temp = 0;
  byte humi = 0;
  int err = SimpleDHTErrSuccess;
  if ((err = dht.read(&temp, &humi, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("Read DHT11 failed, err="); Serial.println(err);
    return -1;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" H");

  *temperature = temp;
  *humidity = humi;
  return SENSOR_DATA_ACQUISITION_SUCCESS;
}

void post_station_data(String url, String stationId, float temperature, float humidity) {
  String body = "{\"stationId\":\"" + stationId + "\",\"temperature\":" + temperature + ",\"humidity\":" + humidity + "}";
  http_post(url, body, "application/json");
}

/**
 * Performs an http post to the given url with the given body and content type.
 * Automatically the Authorization header is added, containing the (base64-encoded)
 * HMAC SHA265 keyed hash of the whole body.
 */
void http_post(String url, String body, String contentType) {
  // calculate the hmac sha256 hash of the body we're gonna send.
  uint8_t hash[SHA256HMAC_SIZE];
  hmac_sha256(hmac_key, HMAC_KEY_LENGTH, body, hash);
  String base64_hash = base64::encode((const char *)hash);
    
  Serial.println("POST: url[" + url + "] body [" + body + "] Authorization [" + base64_hash + "]");
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", contentType);
  http.addHeader("Authorization", base64_hash);
  http.POST(body);
  http.writeToStream(&Serial);
  http.end();
  Serial.println("POST: done");
}

/**
 * @message should be a simple arduino string
 * @hash must be at least SHA256HMAC_SIZE long
 */
void hmac_sha256(uint8_t *key, size_t key_len, String message, byte *hash) {  
  /* Create the HMAC instance with our key */
  SHA256HMAC hmac(key, key_len);
  
  /* Update the HMAC with just a plain string (null terminated) */
  hmac.doUpdate((const char *)message.c_str(), message.length());

  /* Finish the HMAC calculation and fill up the hash buffer */
  hmac.doFinal(hash);
}


int max(int a, int b) {
  if (a > b) {
    return a;
  }
  return b;
}
