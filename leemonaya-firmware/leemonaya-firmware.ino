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

#include <ESP8266WiFi.h>            // https://github.com/esp8266/Arduino and https://arduino-esp8266.readthedocs.io/en/latest/index.html
#include <ESP8266HTTPClient.h>
#include <SimpleDHT.h>              // https://github.com/winlinvip/SimpleDHT/
#include <Crypto.h>                 // https://github.com/intrbiz/arduino-crypto note that this is esp2866 specific!
#include <base64.h>

/*
 * Configuration 
 * All stuff should be self explanatory, intervals are in millis.
 * 
 * The HMAC_KEY_LENGTH and hmac_key can taken by the ingestor server logs.
 * Just set your desidered key in the ingestor server and at startup it will log (to stdout)
 * an actual C snippet to past here.
 * 
 * All strings of the form @XXX@ can be either modified by hand or using the configure script in the root folder.
 */
#define SERIAL_BAUD_RATE 115200
#define WIFI_SSID "@WIFI_SSID@" 
#define WIFI_PASS "@WIFI_PASS@"
#define WIFI_RETRY_INTERVAL 500
#define INITIAL_SETUP_DELAY 2000

#define SENSOR_DATA_POST_URL "@HTTP_SERVER_HOST@:@HTTP_SERVER_PORT@/station-data"
#define STATION_ID "limonaia"
#define SAMPLE_INTERVAL 5000

#define HMAC_KEY_LENGTH 63
static uint8_t hmac_key[HMAC_KEY_LENGTH] = { 99, 97, 115, 100, 99, 97, 115, 100, 99, 97, 115, 100, 107, 106, 
                                            110, 49, 50, 108, 51, 107, 106, 110, 52, 49, 50, 108, 107, 106, 
                                            100, 110, 49, 108, 107, 106, 110, 99, 107, 97, 106, 115, 100, 49, 
                                            50, 51, 52, 117, 104, 56, 99, 104, 57, 99, 104, 49, 119, 115, 106, 
                                            104, 118, 49, 99, 111, 56 };

/* The GPIO pin where the DHT(11|22) sensor is connected. */
#define DHT_PIN 2

/* configuration end here */


#define SENSOR_READ_OK 0
#define SENSOR_READ_KO -1

static SimpleDHT11 dht(DHT_PIN);


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
  
  byte temp = 0;
  byte humi = 0;
  
  int err = SimpleDHTErrSuccess;
  if ((err = dht.read(&temp, &humi, NULL)) != SimpleDHTErrSuccess) {
    Serial.print("-dht sampling-, err="); Serial.println(err);
    return -1;
  }

  *temperature = temp;
  *humidity = humi;
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
  uint8_t hash[SHA256HMAC_SIZE];
  hmac_sha256(hmac_key, HMAC_KEY_LENGTH, body, hash);
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
