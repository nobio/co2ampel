#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino_JSON.h>

/* http client:
 *  #include <ESP8266WiFi.h>
 *  #include <ESP8266WiFiMulti.h>
 *  #include <ESP8266HTTPClient.h>
 *  #include <WiFiClientSecureBearSSL.h>
 */

const char* ssid = "WLAN Kabel";
const char* password = "57002120109202250682";
const unsigned long SENSOR_READ_DELAY = 0.4 * 1000;

float temperature = 0.0;
float humidity = 0.0;
float x = 0.0;
long time_last_sensor_read = 0;

ESP8266WebServer server(80);

// ----------------- Setup ------------------------------------------
void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  // ---------------- Setup Wifi ----------------------------------
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("connecting to wifi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("wemoD1mini")) {
    Serial.println("MDNS responder started");
  }

  // ----------------- Router ------------------------------------------
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  // API
  server.on("/data-text", HTTP_GET, handleDataAsJson);
  server.on("/data-json", HTTP_GET, handleDataAsJson);

  // ----------------- Start ------------------------------------------
  time_last_sensor_read = millis();
  server.begin();
  Serial.println("HTTP server started");
}

// ----------------- Loop ------------------------------------------
void loop(void) {
  readDataFromSensor();
  server.handleClient();
  MDNS.update();
}

void readDataFromSensor() {
  if(millis() - time_last_sensor_read > SENSOR_READ_DELAY) {
    time_last_sensor_read = millis();
    x += 0.1;
    temperature = 20 + 10 * sin(x) + (random(-1000, 1000) / 600); 
    humidity = 50 + 20 * cos(x/3) + (random(-1000, 1000) / 400); 

    Serial.print(temperature);Serial.print(" - ");Serial.println(humidity);
  }
} 
// --------------- Route-Handler ----------------------------------------------------
void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
//  digitalWrite(LED_BUILTIN, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
//  digitalWrite(LED_BUILTIN, LOW);
}

void handleDataAsText() {
  // temperatur=12.3:humidity=40.32
  String resp = "temperature=";
  resp += temperature;
  resp += ":humidity=";
  resp += humidity;
  
  Serial.print("response = ");
  Serial.println(resp);
  
  server.send(200, "text/plain", resp);
}

void handleDataAsJson() {
  // {"temperature": 12.3,"humidity": 40.32}
  digitalWrite(LED_BUILTIN, HIGH);
  JSONVar dataObj;

  dataObj["temperature"] = roundf(temperature * 100.0) / 100.0;
  dataObj["humidity"] = roundf(humidity * 100.0) / 100.0;

  Serial.print("response = ");
  Serial.println(dataObj);
  digitalWrite(LED_BUILTIN, LOW);

  server.send(200, "application/json", JSON.stringify(dataObj));
}
