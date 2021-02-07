#include "bsec.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino_JSON.h>

/* ----------------------WLAN Connect Daten -------------------------- */ 
const char* ssid = "WLAN Kabel";
const char* password = "57002120109202250682";

/* --------------------------- RGB-LED ------------------------------- */
const int PIN_RGB_RED   = D5;
const int PIN_RGB_GREEN = D6;
const int PIN_RGB_BLUE  = D7;

// Helper functions declarations
void checkIaqSensorStatus(void);
void errLeds(void);
void writeLog();

// wifi & router functions
void handleRoot(void);
void handleNotFound(void);
void handleDataAsJson(void);
void setupRoutes(void);
void setupWifi(void);

// Air Quality Values
struct AIRQ {
  float temperature;
  float humidity;
  float pressure;
  float iaq;
  int accuracy;
  float co2;
  float voc;
};
AIRQ airQuality = {0.0, 0.0, 0.0, 0.0, 0, 0.0, 0.0};

// RGB-LEDs
struct RGB {
  byte red;
  byte green;
  byte blue;
};

void setLEDColor(RGB);
void blinkLEDColor(RGB);
void blinkLEDColor(RGB, int);
RGB calculateIaqColor(AIRQ);

// Create an object of the class Bsec
Bsec iaqSensor;
// Instance of Webserver
ESP8266WebServer server(80);

String output;

// Entry point for the example
void setup(void)
{
  Serial.begin(115200);
  Wire.begin();

  /* --------------------- Setup Wifi ------------------------*/
  setupWifi();
  /* --------------------- Setup Routes ----------------------*/
  setupRoutes();
  /* --------------------- Start Webserver--------------------*/
  server.begin();
  Serial.println("HTTP server started");
  /* ---------------------- Setup bme680 -------------------- */
  iaqSensor.begin(BME680_I2C_ADDR_SECONDARY, Wire);
  output = "\nBSEC library version " + String(iaqSensor.version.major) + "." + String(iaqSensor.version.minor) + "." + String(iaqSensor.version.major_bugfix) + "." + String(iaqSensor.version.minor_bugfix);
  Serial.println(output);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[10] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
  };

  iaqSensor.updateSubscription(sensorList, 10, BSEC_SAMPLE_RATE_LP);
  checkIaqSensorStatus();

  // setup RGB-LEDs
  Serial.println("setup RGB-LED");
  pinMode(PIN_RGB_RED,   OUTPUT);
  pinMode(PIN_RGB_RED,   INPUT);
  pinMode(PIN_RGB_GREEN, OUTPUT);
  pinMode(PIN_RGB_GREEN, INPUT);
  pinMode(PIN_RGB_BLUE,  OUTPUT);
  pinMode(PIN_RGB_BLUE,  INPUT);
  setLEDColor(RGB{255,0,213});

  // Print the header
  output = "Timestamp [ms], raw temperature [°C], pressure [hPa], raw relative humidity [%], gas [Ohm], IAQ, IAQ accuracy, temperature [°C], relative humidity [%], Static IAQ, CO2 equivalent, breath VOC equivalent";
  Serial.println(output);
}

// Function that is looped forever
void loop(void)
{
  unsigned long time_trigger = millis();
  if (iaqSensor.run()) { // If new data is available
    // store values
    airQuality.temperature = iaqSensor.temperature;
    airQuality.humidity = iaqSensor.humidity;
    airQuality.pressure = iaqSensor.pressure;
    airQuality.iaq = iaqSensor.iaq;
    airQuality.accuracy = iaqSensor.iaqAccuracy;
    airQuality.co2 = iaqSensor.co2Equivalent;
    airQuality.voc = iaqSensor.breathVocEquivalent;

    // serial out log
    writeLog();

    // do some webserver magic...
    server.handleClient();
    MDNS.update();

    // temporary Code
    if(airQuality.accuracy == 0) {
      blinkLEDColor(RGB{255, 255, 255}, 2);
    } else {
      setLEDColor(calculateIaqColor(airQuality));
    }

  } else {
    checkIaqSensorStatus();
  }
}

void writeLog() {
    output = ">";
//    output += String(time_trigger);
    output += ", rTemp: " + String(airQuality.temperature);
    output += ", press: " + String(airQuality.pressure);
    output += ", rHumi: " + String(airQuality.humidity);
    //output += ", gasRe: " + String(airQuality.gasResistance);
    output += ", iaq:   " + String(airQuality.iaq);
    output += ", iaqAc: " + String(airQuality.accuracy);
    output += ", temp:  " + String(airQuality.temperature);
    output += ", humi:  " + String(airQuality.humidity);
    //output += ", stIaq: " + String(airQuality.staticIaq);
    output += ", co2:   " + String(airQuality.co2);
    output += ", voc:   " + String(airQuality.voc);
    
    Serial.println(output);
}

// Helper function definitions
void checkIaqSensorStatus(void)
{
  if (iaqSensor.status != BSEC_OK) {
    if (iaqSensor.status < BSEC_OK) {
      output = "BSEC error code : " + String(iaqSensor.status);
      Serial.println(output);
      while(iaqSensor.status < BSEC_OK) {
        errLeds(); /* Halt in case of failure */
        output = "BSEC error code : " + String(iaqSensor.status);
        Serial.println(output);
        delay(500);
      }
    } else {
      output = "BSEC warning code : " + String(iaqSensor.status);
      Serial.println(output);
    }
  }

  if (iaqSensor.bme680Status != BME680_OK) {
    if (iaqSensor.bme680Status < BME680_OK) {
      output = "BME680 error code : " + String(iaqSensor.bme680Status);
      Serial.println(output);
      while(iaqSensor.bme680Status < BME680_OK < BSEC_OK) {
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

void errLeds(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  delay(100);
  blinkLEDColor(RGB{0, 0, 255});
}

// ---------------- Setup Wifi ----------------------------------
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

// ----------------- Router ------------------------------------------
void setupRoutes() {
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  // API
  server.on("/data-json", HTTP_GET, handleDataAsJson);

  // ----------------- Start ------------------------------------------
  server.begin();
}
// --------------- Route-Handler ----------------------------------------------------
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
  dataObj["pressure"] = roundf(airQuality.pressure * 100.0) / 100.0;
  dataObj["iaq"] = roundf(airQuality.iaq * 100.0) / 100.0;
  dataObj["iaqAccuracy"] = airQuality.accuracy;
  dataObj["co2"] = roundf(airQuality.co2 * 100.0) / 100.0;
  dataObj["voc"] = roundf(airQuality.voc * 100.0) / 100.0;

  Serial.print("response = ");
  Serial.println(dataObj);
  digitalWrite(LED_BUILTIN, LOW);

  server.send(200, "application/json", JSON.stringify(dataObj));
}

void setLEDColor(RGB color) {
  if(color.red < 0)     color.red = 0;
  if(color.red > 255)   color.red = 255;

  if(color.green < 0)   color.green = 0;
  if(color.green > 255) color.green = 255;

  if(color.blue < 0)    color.blue = 0;
  if(color.blue > 255)  color.blue = 255;

  analogWrite(PIN_RGB_RED,   color.red);
  analogWrite(PIN_RGB_GREEN, color.green);
  analogWrite(PIN_RGB_BLUE,  color.blue);
}

void blinkLEDColor(RGB color, int cycles) {
  for(int n = 0; n < cycles; n++) {
    blinkLEDColor(color);
  }
}

void blinkLEDColor(RGB color) {
  analogWrite(PIN_RGB_RED,   0);
  analogWrite(PIN_RGB_GREEN, 0);
  analogWrite(PIN_RGB_BLUE,  0);
  delay(200);

  analogWrite(PIN_RGB_RED,   color.red);
  analogWrite(PIN_RGB_GREEN, color.green);
  analogWrite(PIN_RGB_BLUE,  color.blue);
  delay(150);
}

RGB calculateIaqColor(AIRQ airQuality) {
  float iaq = airQuality.iaq;
  RGB color = { 0, 0, 0 };

  if(iaq <= 50) {
    color = { 0, 255, 0 };     // green -> good
  } else if(iaq <= 100) {
    color = { 255, 255, 0 };   // yellow -> average
  } else if(iaq <= 150) {
    color = { 255, 173, 51 };  // orange -> little bad
  } else if(iaq <= 200) {
    color = { 255, 0, 0 };     // red  -> bad
  } else if(iaq <= 300) {
    color = { 255, 0, 255 };   // purple -> worse
  } else if(iaq <= 500) {
    color = { 0, 0, 255 };     // blue -> really bad
  }

  return color;
}
