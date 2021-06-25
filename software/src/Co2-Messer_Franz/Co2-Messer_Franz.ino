#include "bsec.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino_JSON.h>
#include <U8g2lib.h>

//Objekte initialisieren:
Bsec iaqSensor;
ESP8266WebServer server(80);

//Wifi-Connection:
const char* ssid = "WLAN Kabel";
const char* password = "57002120109202250682";

// Air Quality Value Structure
struct AIRQ {
  float temperature;
  float humidity;
  float iaq;
  int accuracy;
  float co2;
};
AIRQ airQuality = {0.0, 0.0, 0.0, 0, 0.0};


//LED:
const int led_red   = D5;
const int led_yellow = D6;
const int led_green  = D7;

//sonstiges:
String output_kopf;
String output_daten;
String output;


void setup(void)
{
  Serial.begin(115200);
  Wire.begin();

//Wifi
  setupWifi();
  setupRoutes();
  server.begin();
  Serial.println("HTTP server started");

//Setup-BME680:
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();
  bsec_virtual_sensor_t sensorList[5] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };
  iaqSensor.updateSubscription(sensorList, 5, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

// setup RGB-LEDs
  pinMode(led_red, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_green,  OUTPUT);

  // Print the header
  output_kopf = "temperature [°C], relative humidity [%], IAQ, , CO2 equivalent [ppm], IAQ accuracy";
  Serial.println(output_kopf);
}


void loop(void)
{

//lesen der Werte des Sensors und übertragen auf Variablen:
  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { 
    airQuality.temperature = iaqSensor.temperature;
    airQuality.humidity = iaqSensor.humidity;
    airQuality.iaq = iaqSensor.iaq;
    airQuality.co2 = iaqSensor.co2Equivalent;
    airQuality.accuracy = iaqSensor.iaqAccuracy;
    writeLog();     //Ausgabe auf serial-Monitor 

    //Prüfen der Sensor-accuracy:
    if (airQuality.accuracy == 0) {
      waiting_LED();
    } else {
      calculateIaqColor(airQuality);
      }
    //Schicken der Daten an Web-Server:
    server.handleClient();
    MDNS.update();


  } 
  else {
    checkIaqSensorStatus();
  }
}

/*-------Ausgabe-Methoden:------*/

  //Ausgabe auf Seriellen-Monitor:
void writeLog() {
  output_daten = "# ";
  output_daten += " temp:" + String(airQuality.temperature) + " [°C]";
  output_daten += ", humi:" + String(airQuality.humidity) + " [%]";
  output_daten += ", iaq:" + String(airQuality.iaq);
  output_daten += ", co2:" + String(airQuality.co2) + " ppm";
  output_daten += ", iaqAc:" + String(airQuality.accuracy);
  Serial.println(output_daten);
}

/*-------Sensor-Methoden:------*/
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      errLeds();
      while (iaqSensor.status < BSEC_OK) {
        errLeds(); /* Halt in case of failure */
        output = "BSEC error code : " + String(iaqSensor.status);
        Serial.println(output);
        delay(500);
      }
    } 
    else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      errLeds();
      while (iaqSensor.bme680Status < BME680_OK < BSEC_OK) {
        errLeds(); /* Halt in case of failure */
        output = "BSEC error code : " + String(iaqSensor.status);
        Serial.println(output);
        delay(500);
      }
    } else {
      output = "BME680 warning code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
    }
  }
}

/*-------LEDS-Methoden:------*/

void errLeds(void) {
  digitalWrite(led_red,HIGH);
  delay(200);
  digitalWrite(led_red, LOW);
  delay(200);
}

void waiting_LED() {
  for (int n = 0; n < 3; n++) {
  digitalWrite(led_red, HIGH);
  digitalWrite(led_yellow, HIGH);
  digitalWrite(led_green, HIGH);
  delay(150);

  digitalWrite(led_red, LOW);
  digitalWrite(led_yellow, LOW);
  digitalWrite(led_green, LOW);
  delay(150);
  }
}

void calculateIaqColor(AIRQ airQuality) {
  float iaq = airQuality.iaq;

  if (iaq <= 50) {
  digitalWrite(led_green,HIGH); // grün -> sehr gute Luftqualität
  digitalWrite(led_yellow,LOW);
  digitalWrite(led_red,LOW);
  } else if (iaq <= 125) {
  digitalWrite(led_green,LOW);
  digitalWrite(led_yellow,HIGH);  // gelb -> durchschnittliche Luftqualität 
  digitalWrite(led_red,LOW);  
   
  } else if (iaq <= 500) {
  digitalWrite(led_green,LOW);
  digitalWrite(led_yellow,LOW);
  digitalWrite(led_red,HIGH);     // rot -> sehr schlechte Luftqualität
  }
}

/*-------Wifi-Methoden------*/
void setupWifi() {
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
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  // API
  server.on("/data-json", HTTP_GET, handleDataAsJson);

  server.begin();
}

void handleRoot() {
  server.send(200, "text/plain", "goto /data-json");
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
}

void handleDataAsJson() {
  // {"temperature": 12.3,"humidity": 40.32}
  digitalWrite(LED_BUILTIN, HIGH);
  JSONVar dataObj;

  dataObj["temperature"] = roundf(airQuality.temperature * 100.0) / 100.0;
  dataObj["humidity"] = roundf(airQuality.humidity * 100.0) / 100.0;
  dataObj["iaq"] = roundf(airQuality.iaq * 100.0) / 100.0;
  dataObj["co2"] = roundf(airQuality.co2 * 100.0) / 100.0;
  dataObj["iaqAccuracy"] = airQuality.accuracy;

  Serial.print("response = ");
  Serial.println(dataObj);
  digitalWrite(LED_BUILTIN, LOW);

  server.send(200, "application/json", JSON.stringify(dataObj));
}
