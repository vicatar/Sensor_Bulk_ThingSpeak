# Sensor_Bulk_ThingSpeak

Description:
  This code bulk update sensor data to a ThingSpeak channel using an an ESP8266. The code is basd on an Mathworks example.

License:
  GNU General Public License v3.0

Dependencies:
  - DHT library
  - EPS8266Wifi library

Usage Instructions:
  1. Ensure you have DHT sensor connected to correct PINS on the ESP NodeMCU 1.0
  2. Ensure you have a photocell sensor connected to correct PINS on the NodeMCU
  2. Ensure you have set up a ThingSpeak account and created a channel
  3. Replace the constants noted below with your ThingSpeak channel write API key & and ID
  4. Compile and run this code on the NodeMCU to upload DHT and photocell sensor data to ThingSpeak

References:
  - https://www.mathworks.com/help/thingspeak/continuously-collect-data-and-bulk-update-a-thingspeak-channel-using-an-arduino-mkr1000-board-or-an-esp8266-board.html
