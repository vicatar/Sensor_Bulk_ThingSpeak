/*
Author: Victor Lin
Date: 2023-07-19
Version: 1.0.0
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

*/

#include<ESP8266WiFi.h> 
#include <DHT.h> 

//comment out the following to suppress debugging outputs to serial port and conserve power
//#define DEBUG 1

#define A_SIZE 1500            // size the string large enough to hold all the data
char jsonBuffer[A_SIZE] = "["; // Initialize the jsonBuffer to hold data

// Define the WiFi credentials to connect your Arduino board to the network, and initialize the WiFi client library.
const char* ssid     = "YourSSID";
const char* password = "YourPasswd";
WiFiClient client; // Initialize the WiFi client library

// Collect data once every 30 seconds and post data to ThingSpeak channel once every 2 minutes
unsigned long lastConnectionTime = 0; // Track the last connection time
unsigned long lastUpdateTime = 0; // Track the last update time
const unsigned long postingInterval = 120L * 1000L; // Post data every 2 minutes
const unsigned long updateInterval = 30L * 1000L; // Update once every 30 seconds
const unsigned long sleepInterval = 5L * 1000L; // rest for 5 seconds

//Replace CHANNEL_WRITEAPIKEY with your ThingSpeak channel write API key
//Replace CHANNEL_ID with your ThingSpeak channel ID
const char* server = "api.thingspeak.com";
const char* CHANNEL_WRITEAPIKEY   = "YourCHANNEL";
const int   CHANNEL_ID            = 000000;

/* Define sensor/led variables */
#define LEDPIN   D4
#define DHTPIN   D1
#define DHTTYPE  DHT22
#define PHOTOPIN 0

int photo;
float humidity;
float temperature;
DHT dht(DHTPIN, DHTTYPE, 15);

void setup() {
  pinMode(LEDPIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  Serial.begin(115200);
  // Initialize sensor
  dht.begin();
  photo = analogRead(PHOTOPIN);

  // Attempt to connect to WiFi network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);  // Connect to WPA/WPA2 network. Change this line if using open or WEP network
    delay(5000);  // Wait 5 seconds to connect
  }
  Serial.println("Connected to wifi");

  printWiFiStatus(); // Print WiFi connection information
}

void loop() {
  // If update time has reached updateInterval seconds, then update the jsonBuffer
  if (millis() - lastUpdateTime >=  updateInterval) {
     
#if DEBUG
    printRSSI();
#endif
    updatesJson(jsonBuffer);
  }

  // Heartbeat LED
  digitalWrite(LEDPIN, LOW);  // Turn the LED on by making the voltage HIGH
  delay(500);                 // for 0.5 sec
  digitalWrite(LEDPIN, HIGH);  // Turn the LED off by making the voltage HIGH
  delay(sleepInterval - 500);
}

// Updates the jsonBuffer with data
void updatesJson(char* jsonBuffer){
  /* JSON format for updates paramter in the API
   *  This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can provide the absolute timestamp using the "created_at" parameter
  *  instead of "delta_t".
   *  "[{\"delta_t\":0,\"field1\":-70},{\"delta_t\":15,\"field1\":-66}]"
   */
  // Read sensor values
  readSensors();
  if (isnan(humidity) || isnan(temperature)) {
    return;
  }

#if DEBUG
  Serial.println("Update");  
  printSensors();
#endif

  // Format the jsonBuffer as noted above
  strcat(jsonBuffer,"{\"delta_t\":");
  unsigned long deltaT = (millis() - lastUpdateTime)/1000;
  size_t lengthT = String(deltaT).length();
  char temp[8];
  String(deltaT).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");

  strcat(jsonBuffer, "\"field1\":");
  lengthT = String(temperature).length();
  if ( lengthT > 5 )
    lengthT = 5;
  String(temperature).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");

  strcat(jsonBuffer, "\"field2\":");
  lengthT = String(humidity).length();
  if ( lengthT > 5 )
    lengthT = 5;
  String(humidity).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");

  strcat(jsonBuffer, "\"field3\":");
  lengthT = String(photo).length();
  if ( lengthT > 5 )
    lengthT = 5;
  String(photo).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,"},");
  // If posting interval time has reached 2 minutes, then update the ThingSpeak channel with your data
  if (millis() - lastConnectionTime >=  postingInterval) {
        size_t len = strlen(jsonBuffer);
        jsonBuffer[len-1] = ']';
        httpRequest(jsonBuffer);
  }
  lastUpdateTime = millis(); // Update the last update time
}

// Updates the ThingSpeakchannel with data
void httpRequest(char* jsonBuffer) {
  /* JSON format for data buffer in the API
   *  This example uses the relative timestamp as it uses the "delta_t". If your device has a real-time clock, you can also provide the absolute timestamp using the "created_at" parameter
   *  instead of "delta_t".
   *   "{\"write_api_key\":\"CHANNEL_WRITEAPIKEY\",\"updates\":[{\"delta_t\":0,\"field1\":-60},{\"delta_t\":15,\"field1\":200},{\"delta_t\":15,\"field1\":-66}]
   */
   
  // Format the data buffer as noted above
  char data[A_SIZE] = "{\"write_api_key\":\"";
  strcat(data,CHANNEL_WRITEAPIKEY);
  strcat(data,"\",\"updates\":"); 
  strcat(data,jsonBuffer);
  strcat(data,"}");
#if DEBUG
  Serial.println(data);
#endif

  // Close any connection before sending a new request
  client.stop();

  String data_length = String(strlen(data)+1); //Compute the data buffer length
  // POST data to ThingSpeak
  if (client.connect(server, 80)) {
    client.println("POST /channels/" + String(CHANNEL_ID) + "/bulk_update.json HTTP/1.1"); 
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: "+data_length);
    client.println();
    client.println(data);
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); //Wait to receive the response
  client.parseFloat();
  String resp = String(client.parseInt());
#if DEBUG
  Serial.println("Response code:"+resp); // Print the response code. 202 indicates that the server has accepted the response
#endif

  jsonBuffer[0] = '['; // Reinitialize the jsonBuffer for next batch of data
  jsonBuffer[1] = '\0';
  lastConnectionTime = millis(); // Update the last conenction time
}

void printWiFiStatus() {
  // Print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // Print your device IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // Print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void printRSSI()
{
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void printSensors()
{
  Serial.print("temperature: ");
  Serial.println(temperature);
  Serial.print("humidity: ");
  Serial.println(humidity);
  Serial.print("photo: ");
  Serial.println(photo);
}

/* 
 * Read the sensors DHT and photoresistor 
 */
void readSensors() {
  int i;
  int val;
  humidity = dht.readHumidity();
  temperature = dht.readTemperature(true);
  // compute sliding average of the last two photoresistor measurements
  val = analogRead(PHOTOPIN);
  photo = (photo + val) >> 1;
}
