// Example testing sketch for various DHT humidity/temperature sensors
// Written by ladyada, public domain

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>


#define LED_BUILTIN 2

#define DHTPIN 5     // Digital pin connected to the DHT sensor
// Feather HUZZAH ESP8266 note: use pins 3, 4, 5, 12, 13 or 14 --
// Pin 15 can work but DHT must be disconnected during program upload.

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.
DHT dht(DHTPIN, DHTTYPE);

// Replace with your network details
const char* ssid = "WIFISSID";
const char* password = "WIFIPASSWORD";

const char* iotServerUrl = "http://192.168.42.44:5000/Dht/Get";
const char* iotStationId = "station-id1";

// Web Server on port 80
WiFiServer server(80);

// Temporary variables
static char celsiusTemp[7];
static char humidity[7];
static char url[100];

static int loopCounter;

void blinkBuiltin(int interval) {
  digitalWrite(LED_BUILTIN, LOW);
  delay(interval);
  digitalWrite(LED_BUILTIN, HIGH);
}

void serialPrint(char* temperature, char* humidity)
{
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t Temperature: ");
  Serial.print(temperature);
  Serial.println(" *C");
}

void sendViaHttp(float t, float h) {
  blinkBuiltin(50);
  delay(10);
  blinkBuiltin(50);
  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");

  sprintf(url, "%s?i=%s&t=%.2f&h=%.0f", iotServerUrl, iotStationId, t, h); 

  if (http.begin(client, url)) {  // HTTP

    Serial.print("[HTTP] GET... ");
    Serial.println(url);
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();
        Serial.println(payload);
      }
      blinkBuiltin(20);
      delay(100);
      blinkBuiltin(20);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      blinkBuiltin(100);
      delay(100);
      blinkBuiltin(100);
      delay(100);
      blinkBuiltin(100);
    }

    http.end();
  } else {
    Serial.printf("[HTTP] Unable to connect\n");
    blinkBuiltin(100);
    delay(100);
    blinkBuiltin(100);
    delay(100);
    blinkBuiltin(100);
    delay(100);
    blinkBuiltin(100);
    delay(100);
    blinkBuiltin(100);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  loopCounter = 0;
  // Initializing serial port for debugging purposes
  Serial.begin(115200);
  delay(10);

  Serial.print("Station ID: ");
  Serial.println(iotStationId);

  dht.begin();
  
  // Connecting to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Starting the web server
  server.begin();
  Serial.println("Web server running. Waiting for the ESP IP...");
  delay(10000);
  
  // Printing the ESP IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(10); 
  ++loopCounter;

  if (loopCounter > 6000) // every 60 seconds
  {
    loopCounter = 0;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      strcpy(celsiusTemp,"Failed");
      strcpy(humidity, "Failed");         
    }
    else{
      // Computes temperature values in Celsius + Fahrenheit and Humidity
      dtostrf(h, 6, 2, humidity);
      dtostrf(t, 6, 2, celsiusTemp);
      serialPrint(celsiusTemp, humidity);
      sendViaHttp(t, h);
    }
  }
  
  // Listenning for new clients
  WiFiClient client = server.available();
  
  if (client) {
    Serial.println("New client");
    blinkBuiltin(50);
    // bolean to locate when the http request ends
    boolean blank_line = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        if (c == '\n' && blank_line) {
            // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
            float h = dht.readHumidity();
            // Read temperature as Celsius (the default)
            float t = dht.readTemperature();
            // Check if any reads failed and exit early (to try again).
            if (isnan(h) || isnan(t)) {
              Serial.println("Failed to read from DHT sensor!");
              strcpy(celsiusTemp,"Failed");
              strcpy(humidity, "Failed");         
            }
            else{
              // Computes temperature values in Celsius + Fahrenheit and Humidity
              dtostrf(h, 6, 2, humidity);
              dtostrf(t, 6, 2, celsiusTemp);
              serialPrint(celsiusTemp, humidity);
            }

            client.println("HTTP/1.1 200 OK");
            client.println("Content-Type: text/html");
            client.println("Connection: close");
            client.println();
            // your actual web page that displays temperature and humidity
            client.println("<!DOCTYPE HTML>");
            client.println("<html>");
            client.println("<head></head><body><h1>ESP8266 - Temperature and Humidity</h1><h3>Temperature in Celsius: ");
            client.println(celsiusTemp);
            client.println("*C</h3><h3>Humidity: ");
            client.println(humidity);
            client.println("%</h3><h3>");
            client.println("</body></html>");     
            break;
        }
        if (c == '\n') {
          // when starts reading a new line
          blank_line = true;
        }
        else if (c != '\r') {
          // when finds a character on the current line
          blank_line = false;
        }
      }
    }  
    // closing the client connection
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
    blinkBuiltin(50);
    delay(50);
    blinkBuiltin(100);
  }
}

