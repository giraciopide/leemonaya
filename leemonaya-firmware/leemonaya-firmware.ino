/*
 * Copyright (c) 2018 Marco Nicolini
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/***************************************************************/
/* configuration starts here                                   */
/***************************************************************/

/*
 * All stuff should be self explanatory, intervals are in millis.
 * 
 * The HMAC_KEY_LENGTH and hmac_key can taken by the ingestor server logs.
 * Just set your desidered key in the ingestor server and at startup it will log (to stdout)
 * an actual C snippet to past here. 
 */
#define SERIAL_BAUD_RATE 9600
#define WIFI_SSID "enter_your_wifi_ssid_here" 
#define WIFI_PASS "enter_your_wifi_pass_here"
#define WIFI_RETRY_INTERVAL 500
#define INITIAL_SETUP_DELAY 15000

#define SENSOR_DATA_POST_URL "http://localhost:5000/station-data"
#define STATION_ID "limonaia"
#define SAMPLE_INTERVAL 30000

#define SHA256_HMAC_LEN 32
#define HMAC_KEY_LEN 60

static uint8_t hmac_key[HMAC_KEY_LEN] = {99, 97, 115, 100, 97, 115, 100, 99, 115, 100, 107, 110, 49, 50, 108, 51, 107, 106, 110, 52, 49, 50, 108, 107, 106, 100, 110, 49, 108, 107, 106, 110, 99, 107, 97, 106, 115, 100, 49, 50, 51, 52, 117, 104, 56, 99, 104, 57, 99, 104, 49, 119, 115, 106, 104, 118, 49, 99, 111, 56};

#define USE_DHT22
/* The GPIO pin where the DHT(11|22) sensor is connected. */
#define DHT_PIN 16

/***************************************************************/
/* configuration ends here                                     */
/***************************************************************/

#include <ESP8266WiFi.h>            // https://github.com/esp8266/Arduino and https://arduino-esp8266.readthedocs.io/en/latest/index.html
#include <ESP8266HTTPClient.h>
#include <SimpleDHT.h>              // https://github.com/winlinvip/SimpleDHT/

#include <Crypto.h>                 // https://github.com/rweather/arduinolibs
#include <SHA256.h>                 // https://github.com/rweather/arduinolibs

#include <base64.h>

#define SENSOR_READ_OK 0
#define SENSOR_READ_KO -1

#ifdef USE_DHT11
static SimpleDHT11 dht(DHT_PIN);
#endif

#ifdef USE_DHT22
static SimpleDHT22 dht(DHT_PIN);
#endif

void setup(){
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("+setup+");
  delay(INITIAL_SETUP_DELAY);
  Serial.println("Initializing WIFI...");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Polling until Wifi is UP...");
  poll_until_wifi_up(WIFI_RETRY_INTERVAL);
  Serial.println("-setup-");
}


void loop() {
  Serial.println("+loop+");
  int start_time = millis();
  poll_until_wifi_up(WIFI_RETRY_INTERVAL);
  
  float temperature = 0;
  float humidity = 0;

  int res = SENSOR_READ_OK;
  if ((res = read_sensor_data(&temperature, &humidity)) == SENSOR_READ_OK) {
      post_station_data(SENSOR_DATA_POST_URL, STATION_ID, temperature, humidity);
  }
  Serial.println("-loop-");

  delay_up_to(start_time, SAMPLE_INTERVAL);
}


/**
 * Polls the wifi connection every poll_interval millis until it's up.
 */
void poll_until_wifi_up(int poll_interval) {
  while (WiFi.status() != WL_CONNECTED) {
    delay(poll_interval);
    Serial.print("X");
  }
  Serial.print("Local ip is: "); Serial.println(WiFi.localIP());
}


/**
 * Reads the temperature and humidity from the DHT11 / DHT22 sensors.
 */
int read_sensor_data(float *temperature, float *humidity) {
  Serial.println("+dht sampling+");

  int err = SimpleDHTErrSuccess;
  if ((err = dht.read2(temperature, humidity, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("-dht sampling-, err="); Serial.println(err);
    return SENSOR_READ_KO;
  }

  Serial.println("-dht sampling-");
  return SENSOR_READ_OK;
}


void post_station_data(String url, String stationId, float temperature, float humidity) {
  String body = "{\"stationId\":\"" + stationId + "\",\"temperature\":" + temperature + ",\"humidity\":" + humidity + "}";
  http_post(url, body, "application/json", "Authorization");
}


/**
 * Performs an http post to the given url with the given body and content type.
 * Automatically a header is added, containing the (base64-encoded)
 * HMAC SHA265 keyed hash of the whole body.
 */
void http_post(String url, String body, String contentType, String hmac256_header) {
  Serial.println("+http_post+");
  // calculate the hmac sha256 hash of the body we're gonna send.
  uint8_t hash[SHA256_HMAC_LEN];
  hmac_sha256(hmac_key, HMAC_KEY_LEN, body, hash);
  String base64_hash = base64::encode((const char *)hash);

  // actually perform the HTTP call.
  Serial.println("POST: url[" + url + "] body [" + body + "] " + hmac256_header + " [" + base64_hash + "]");
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", contentType);
  http.addHeader(hmac256_header, base64_hash);
  http.POST(body);
  http.writeToStream(&Serial);
  http.end();
  Serial.println("-http_post-");
}


void hmac_sha256(uint8_t *key, size_t key_len, String message, byte *hmac_hash) {
  Serial.println("performing sha256 hmac hashing...");
  SHA256 hasher;
  hasher.resetHMAC((const void *)key, key_len);
  hasher.update((const void *)message.c_str(), message.length());
  hasher.finalizeHMAC((const void *)key, sizeof(key), (void *)hmac_hash, SHA256_HMAC_LEN);
  Serial.println("done");
}


/**
 * Sleeps the amount of time (low capped at 0) needed to wake up when the provided interval
 * elapses from the given start time.
 */
void delay_up_to(unsigned long start_time, unsigned long interval) {
    delay(max(interval - (millis() - start_time), 0));
}


int max(int a, int b) {
  return a > b ? a : b;
}
