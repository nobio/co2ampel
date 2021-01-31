#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
 
 
#define SEALEVELPRESSURE_HPA (1013.25)
 
Adafruit_BME680 bme; // I2C

/* .............. define class variables ..................... */
float g_humScore, g_gasScore;
float g_gasReference = 250000;
float g_humReference = 40;
int   g_getgasreferenceCounter = 0;
/* ........................................................... */

 
void setup() {
  Serial.begin(115200);
  while (!Serial);
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
}
 
void loop() 
{
  if (! bme.performReading()) 
  {
    Serial.println("Failed to perform reading :(");
    return;
  }
  Serial.print("Temperature = ");
  Serial.print(bme.temperature);
  Serial.println(" *C");
 
  Serial.print("Pressure = ");
  Serial.print(bme.pressure / 100.0);
  Serial.println(" hPa");
 
  Serial.print("Humidity = ");
  Serial.print(bme.humidity);
  Serial.println(" %");
 
  Serial.print("Gas = ");
  Serial.print(bme.gas_resistance / 1000.0);
  Serial.println(" KOhms");

  Serial.print("IAQ = ");
  Serial.print(getAirQuality());
  Serial.println("");
 
  Serial.print("IAQ Score = ");
  Serial.print(getAirQualityScore());
  Serial.println("");
 
  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");
 
  Serial.print("Humidity Score = ");
  Serial.println(g_humScore);
  Serial.print("Gas Score = ");
  Serial.println(g_gasScore);
  Serial.println();
  delay(2000);
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
