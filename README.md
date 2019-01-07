# leemonaya

A remote temperature and humidity monitoring system for my girlfriend's greenhouse, which houses a small citrus grove.

The idea is that periodically, 
- an IOT board, reads temperature/humidity sensor data
- the IOT board (over wifi) performs an HTTP POST call to report its status (board id and sensor readings)
- the http ingestion server authenticates the incoming sensor reading and persists them to a database

Includes:
- firmware for a ESP8266E Wifi board, to be compiled and flashed via the Arduino IDE
- http server to ingest sensor readings (over HTTP POST) and store them in database (2 versions! One in python/flask and the other in node/typescript)
- a minimalistic web single page application ui, made in typescript + webpack + chart.js

Authentication is performed by means of a shared secret key that is used both in client (hardcoded in the ESP8266 firwmare)
and server to calculate the HMAC SHA256 hash of the whole post request body. 
The base64 encoded hash of the request body is passed in the http 'Authorization' header.

IOT Hardware:
- The boards i'm using: https://wiki.wemos.cc/products:d1:d1 and http://blog.jeronimus.net/2016/07/esp8266-wangtongze-arduino-uno-shield.html
- DHT11 and DHT22, Digital temp/humidity sensors: https://cdn-learn.adafruit.com/downloads/pdf/dht.pdf
- A USB to TTL adapter to connect to the board serial interface to flash the firmware (not needed for the Wemos d1 r2 board, since it has onboard USB)
  I used this: http://www.dsdtech-global.com/search/label/USB%20to%20TTL%20Serial%20Converter

IOT Software/Firmware used:
- Arduino IDE: https://www.arduino.cc/
- ESP8266 support for Arduino: https://github.com/esp8266/Arduino
  and its doc https://arduino-esp8266.readthedocs.io/en/latest/index.html
- SimpleDHT, a library to ease reading from the DHT* sensor: https://github.com/winlinvip/SimpleDHT/
- a library to calculate HMAC SHA256 on ESP8266 - https://github.com/intrbiz/arduino-crypto

Ingestion web server:
- made in python3/flask
- current db is sqlite3 (other databases support is trivial, as long as they have a compliant python3 db api).

Not yet implemented:
- web interface to view/explore the data
- https, to eliminate vulnerabilities to message replay
