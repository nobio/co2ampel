#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino_JSON.h>
 
 
#define SEALEVELPRESSURE_HPA (1013.25)

const char* ssid = "WLAN Kabel";
const char* password = "57002120109202250682";

const unsigned long SENSOR_READ_DELAY = 2 * 1000;

/* .............. define class variables ..................... */
float g_humScore, g_gasScore;
float g_gasReference = 250000;
float g_humReference = 40;
int   g_getgasreferenceCounter = 0;
/* ........................................................... */

Adafruit_BME680 bme; // I2C
ESP8266WebServer server(80);

float temperature = 0.0;
float humidity = 0.0;
float pressure = 0.0;
float iaqQualityScore = 0.0;
float gasResistance = 0.0;

long time_last_sensor_read = 0;

 
void setup() {
  Serial.begin(115200);
  while (!Serial);

  /* ------------------ BME680 ---------------------------------------------- */
  Serial.println(F("BME680 test"));
 
  if (!bme.begin(0x77)) 
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
 
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  /* --------------------- Setup Wifi ------------------------*/
  setupWifi();
  /* --------------------- Setup Routes ------------------------*/
  setupRoutes();
  /* --------------------- Start Webserver------------------------*/
  time_last_sensor_read = millis();
  server.begin();
  Serial.println("HTTP server started");

}
 
void loop() 
{
  readDataFromSensor();
  server.handleClient();
  MDNS.update();
  
  delay(2000);
}
void setupWifi() {
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
}

void setupRoutes() {
  // ----------------- Router ------------------------------------------
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  // API
  server.on("/data-json", HTTP_GET, handleDataAsJson);

  // ----------------- Start ------------------------------------------
  time_last_sensor_read = millis();
  server.begin();
}

void readDataFromSensor() {
  if(millis() - time_last_sensor_read > SENSOR_READ_DELAY) {
    time_last_sensor_read = millis();
  
    if (! bme.performReading()) 
    {
      Serial.println("Failed to perform reading :(");
      return;
    }
    temperature = bme.temperature;
    humidity = bme.humidity;
    pressure = bme.pressure;
    gasResistance = bme.gas_resistance / 1000.0;
    iaqQualityScore = getAirQualityScore();
    
    Serial.print("Temperature = ");
    Serial.print(temperature);
    Serial.println(" *C");
   
    Serial.print("Pressure = ");
    Serial.print(pressure / 100.0);
    Serial.println(" hPa");
   
    Serial.print("Humidity = ");
    Serial.print(humidity);
    Serial.println(" %");
   
    Serial.print("Gas = ");
    Serial.print(gasResistance);
    Serial.println(" KOhms");
  /*
    Serial.print("IAQ = ");
    Serial.print(getAirQuality());
    Serial.println("");
 */
    Serial.print("IAQ Score = ");
    Serial.print(iaqQualityScore);
    Serial.println("");
   /*
    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");
   */
   /*
    Serial.print("Humidity Score = ");
    Serial.println(g_humScore);
    Serial.print("Gas Score = ");
    Serial.println(g_gasScore);
    */
    Serial.println();
  }
}

String getAirQuality() {
   return calculateIAQ(getAirQualityScore());
}

float getAirQualityScore() {
 
    float currentHumidity = bme.readHumidity();
    if (currentHumidity >= 38 && currentHumidity <= 42) {
        g_humScore = 0.25 * 100; // Humidity +/-5% around optimum
    } else { //sub-optimal
        if (currentHumidity < 38) {
            g_humScore = 0.25 / g_humReference * currentHumidity * 100;
        } else {
            g_humScore = ((-0.25 / (100 - g_humReference) * currentHumidity) + 0.416666) * 100;
        }
    }

  //Calculate gas contribution to IAQ index
  float gasLowerLimit = 5000;   // Bad air quality limit
  float gasUpperLimit = 50000;  // Good air quality limit 
  
  if (g_gasReference > gasUpperLimit) g_gasReference = gasUpperLimit; 
  if (g_gasReference < gasLowerLimit) g_gasReference = gasLowerLimit;

  g_gasScore = (0.75 / (gasUpperLimit - gasLowerLimit) * g_gasReference - (gasLowerLimit * (0.75 / (gasUpperLimit - gasLowerLimit)))) * 100;
  float airQualityScore = g_humScore + g_gasScore;
  if ((g_getgasreferenceCounter++)%10==0) getGasReference(); 
  
  return airQualityScore;
}

void getGasReference(){
  int readings = 10;
  for (int i = 1; i <= readings; i++){
    g_gasReference += bme.readGas();
  }
  g_gasReference = g_gasReference / readings;
}

String calculateIAQ(float score) {
  String IAQ_text = "Air quality is ";
  score = (100 - score) * 5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  return IAQ_text;
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

void handleDataAsJson() {
  // {"temperature": 12.3,"humidity": 40.32}
  digitalWrite(LED_BUILTIN, HIGH);
  JSONVar dataObj;

  dataObj["temperature"] = roundf(temperature * 100.0) / 100.0;
  dataObj["humidity"] = roundf(humidity * 100.0) / 100.0;
  dataObj["pressure"] = roundf(pressure * 100.0) / 100.0;
  dataObj["gasResistance"] = roundf(gasResistance * 100.0) / 100.0;
  dataObj["iaqQualityScore"] = roundf(iaqQualityScore * 100.0) / 100.0;
  dataObj["iaq"] = getAirQuality();

  Serial.print("response = ");
  Serial.println(dataObj);
  digitalWrite(LED_BUILTIN, LOW);

  server.send(200, "application/json", JSON.stringify(dataObj));
}
