#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

float g_humScore, g_gasScore;
float g_gasReference = 250000;
float g_humReference = 40;
int   g_getgasreferenceCounter = 0;

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1000);
  display.clearDisplay();

  if (!bme.begin()) {
    Serial.println("Der Sensor BME680 konnte nicht gefunden werden!");
    while (1);
  }

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}

void loop() {
  if (! bme.performReading()) {
    Serial.println("Failed to perform reading :(");
    return;
  }

  delay(1000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  String temp = "Temp.: ";
  temp += String(bme.temperature, 2);
  temp += "C";

  display.println(temp);

  display.setCursor(0, 8);
  String hum = "Hum.: ";
  hum += String(bme.humidity, 2);
  hum += "%";
  display.println(hum);

  String gas = "Gas: ";
  gas += String(bme.gas_resistance / 1000.0, 2);
  gas += " KOhms";
  display.setCursor(0, 16);
  display.println(gas);

  String airQuality = getAirQuality();
  display.setCursor(0, 24);
  display.println(airQuality);

  display.display();
  display.clearDisplay();
}

String getAirQuality() {
    float current_humidity = bme.readHumidity();
    if (current_humidity >= 38 && current_humidity <= 42) {
        g_humScore = 0.25 * 100; // Humidity +/-5% around optimum
    } else { //sub-optimal
        if (current_humidity < 38) {
            g_humScore = 0.25 / g_humReference * current_humidity * 100;
        } else {
            g_humScore = ((-0.25 / (100 - g_humReference) * current_humidity) + 0.416666) * 100;
        }
    }

  //Calculate gas contribution to IAQ index
  float gas_lower_limit = 5000;   // Bad air quality limit
  float gas_upper_limit = 50000;  // Good air quality limit 
  
  if (g_gasReference > gas_upper_limit) g_gasReference = gas_upper_limit; 
  if (g_gasReference < gas_lower_limit) g_gasReference = gas_lower_limit;

  g_gasScore = (0.75 / (gas_upper_limit - gas_lower_limit) * g_gasReference - (gas_lower_limit * (0.75 / (gas_upper_limit - gas_lower_limit)))) * 100;
  float air_quality_score = g_humScore + g_gasScore;
  if ((g_getgasreferenceCounter++)%10==0) GetGasReference(); 
  
  return CalculateIAQ(air_quality_score);
}

void GetGasReference(){
  int readings = 10;
  for (int i = 1; i <= readings; i++){
    g_gasReference += bme.readGas();
  }
  g_gasReference = g_gasReference / readings;
}

String CalculateIAQ(float score) {
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