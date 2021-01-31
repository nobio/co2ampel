/*
 * It is not allowed to deliver the source code of the Software to any third party without permission of
 * Bosch Sensortec.
 *
 */

/*!
 * @file bsec_iot_example.ino
 *
 * @brief
 * Example for using of BSEC library in a fixed configuration with the BME680 sensor.
 * This works by running an endless loop in the bsec_iot_loop() function.
 */

/*!
 * @addtogroup bsec_examples BSEC Examples
 * @brief BSEC usage examples
 * @{*/

/**********************************************************************************************************************/
/* header files */
/**********************************************************************************************************************/

#include "bsec_integration.h"
#include <Wire.h>

/**********************************************************************************************************************/
/* Hardware Pins */
/**********************************************************************************************************************/
const int LED_GOOD_PIN = 12; // GPIO12 -> D6
const int LED_BAD_PIN = 13;  // GPIO13 -> D7
const int POTI_PIN = A0;

/**********************************************************************************************************************/
/* local variables */
/**********************************************************************************************************************/

/* status */
#define STATE_UNDEFINED INT8_C(-1)
#define STATE_RAMPUP INT8_C(0)
#define STATE_GOOD INT8_C(1)
#define STATE_OK INT8_C(2)
#define STATE_SOLALA INT8_C(3)
#define STATE_BAD INT8_C(4)
#define STATE_REALLYBAD INT8_C(5)
#define STATE_SUPERBAD INT8_C(6)

int8_t state = STATE_RAMPUP;

/* IAQ Accuracy */
#define IAQ_ACCURACY_INIT INT8_C(0)
#define IAQ_ACCURACY_OK INT8_C(1)
#define IAQ_ACCURACY_IN_CALIBRATION INT8_C(2)
#define IAQ_ACCURACY_CALIBRATED INT8_C(3)

/* Wifi SSID and Password */
const char *WLAN_SSID = "WLAN Kabel";
const char *WLAN_PASSWD = "57002120109202250682";

/**********************************************************************************************************************/
/* functions */
/**********************************************************************************************************************/

/*!
 * @brief           Write operation in either Wire or SPI
 *
 * param[in]        dev_addr        Wire or SPI device address
 * param[in]        reg_addr        register address
 * param[in]        reg_data_ptr    pointer to the data to be written
 * param[in]        data_len        number of bytes to be written
 *
 * @return          result of the bus communication function
 */
int8_t bus_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr); /* Set register address to start writing to */

    /* Write the data */
    for (int index = 0; index < data_len; index++)
    {
        Wire.write(reg_data_ptr[index]);
    }

    return (int8_t)Wire.endTransmission();
}

/*!
 * @brief           Read operation in either Wire or SPI
 *
 * param[in]        dev_addr        Wire or SPI device address
 * param[in]        reg_addr        register address
 * param[out]       reg_data_ptr    pointer to the memory to be used to store the read data
 * param[in]        data_len        number of bytes to be read
 *
 * @return          result of the bus communication function
 */
int8_t bus_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *reg_data_ptr, uint16_t data_len)
{
    int8_t comResult = 0;
    Wire.beginTransmission(dev_addr);
    Wire.write(reg_addr); /* Set register address to start reading from */
    comResult = Wire.endTransmission();

    delayMicroseconds(150);                        /* Precautionary response delay */
    Wire.requestFrom(dev_addr, (uint8_t)data_len); /* Request data */

    int index = 0;
    while (Wire.available()) /* The slave device may send less than requested (burst read) */
    {
        reg_data_ptr[index] = Wire.read();
        index++;
    }

    return comResult;
}

/*!
 * @brief           System specific implementation of sleep function
 *
 * @param[in]       t_ms    time in milliseconds
 *
 * @return          none
 */
void sleep(uint32_t t_ms)
{
    delay(t_ms);
}

/*!
 * @brief           Capture the system time in microseconds
 *
 * @return          system_current_time    current system timestamp in microseconds
 */
int64_t get_timestamp_us()
{
    return (int64_t)millis() * 1000;
}

/*!
 * @brief           Load previous library state from non-volatile memory
 *
 * @param[in,out]   state_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to state_buffer
 */
uint32_t state_load(uint8_t *state_buffer, uint32_t n_buffer)
{
    // ...
    // Load a previous library state from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no state was available,
    // otherwise return length of loaded state string.
    // ...
    return 0;
}

/*!
 * @brief           Save library state to non-volatile memory
 *
 * @param[in]       state_buffer    buffer holding the state to be stored
 * @param[in]       length          length of the state string to be stored
 *
 * @return          none
 */
void state_save(const uint8_t *state_buffer, uint32_t length)
{
    // ...
    // Save the string some form of non-volatile memory, if possible.
    // ...
}

/*!
 * @brief           Load library config from non-volatile memory
 *
 * @param[in,out]   config_buffer    buffer to hold the loaded state string
 * @param[in]       n_buffer        size of the allocated state buffer
 *
 * @return          number of bytes copied to config_buffer
 */
uint32_t config_load(uint8_t *config_buffer, uint32_t n_buffer)
{
    // ...
    // Load a library config from non-volatile memory, if available.
    //
    // Return zero if loading was unsuccessful or no config was available,
    // otherwise return length of loaded config string.
    // ...
    return 0;
}

/*!
 * @brief       Main function which configures BSEC library and then reads and processes the data from sensor based
 *              on timer ticks
 *
 * @return      result of the processing
 */
void setup()
{
    return_values_init ret;

    /* Init the PED Pins */
    pinMode(LED_GOOD_PIN, OUTPUT);
    pinMode(LED_BAD_PIN, OUTPUT);
    pinMode(POTI_PIN, INPUT);

    /* Init I2C and serial communication */
    Wire.begin(0x77);
    //Wire.begin(0, 2);
    Serial.begin(115200);

    /* Call to the function which initializes the BSEC library 
     * Switch on low-power mode and provide no temperature offset */
    ret = bsec_iot_init(BSEC_SAMPLE_RATE_LP, 0.0f, bus_write, bus_read, sleep, state_load, config_load);
    if (ret.bme680_status)
    {
        /* Could not intialize BME680 */
        Serial.println("Error while initializing BME680");
        return;
    }
    else if (ret.bsec_status)
    {
        /* Could not intialize BSEC library */
        Serial.println("Error while initializing BSEC library");
        return;
    }

    /* Call to endless loop function which reads and processes data based on sensor settings */
    /* State is saved every 10.000 samples, which means every 10.000 * 3 secs = 500 minutes  */
    bsec_iot_loop(sleep, get_timestamp_us, output_ready, state_save, 10000);
}

void loop()
{
    // nothing to do; all magic happens in bsec_iot_loop which calls back to output_ready(...)
}

/* ========================================================================== */
/*                                 LOCAL FUNCTIONS
/* ========================================================================== */

void handle_led()
{
    String strState = "unknwon";
    /* --------------- STATE_UNDEFINED -------------------- */
    if (state == STATE_UNDEFINED)
    {
        strState = "  -> undefined";
        // blink fast STATE
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, LOW);
        delay(150);
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, HIGH);
        delay(150);
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, LOW);
        delay(150);
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, HIGH);
        delay(50);
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, LOW);
    }
    /* --------------- RAMPUP -------------------- */
    if (state == STATE_RAMPUP)
    {
        strState = "  -> blink";
        // blink fast STATE
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, HIGH);
        delay(50);
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, LOW);
        delay(50);
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, HIGH);
        delay(50);
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, LOW);
    }
    /* ----------------- GOOD -------------------- */
    else if (state == STATE_GOOD || state == STATE_OK || state == STATE_SOLALA)
    {
        strState = "  -> LED_GOOD on, LED_BAD off";
        // switch on LED_GOOD and switch off LED_BAD
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, LOW);
    }
    /* ---------------- BAD ------------------- */
    else if (state == STATE_BAD || state == STATE_REALLYBAD || state == STATE_SUPERBAD)
    {
        strState = "  -> LED_GOOD off, LED_BAD on";
        // switch off LED_GOOD and switch on LED_BAD
        digitalWrite(LED_GOOD_PIN, LOW);
        digitalWrite(LED_BAD_PIN, HIGH);
    }
    /* ---- SOMETHING MUST HAVE GONE WRONG ---- */
    else
    {
        strState = "  -> ?????";
        // switch off LED_GOOD and switch on LED_BAD
        digitalWrite(LED_GOOD_PIN, HIGH);
        digitalWrite(LED_BAD_PIN, HIGH);
    }
    Serial.println(strState);

    /*
    Serial.print(strState);
    Serial.print(" (state=");
    Serial.print(state);
    Serial.print(")");
    Serial.println();
    */
}
void sendData(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity, float pressure, float triggerValue)
{
}

/* ========================================================================== */
/*                          END OF LOCAL FUNCTIONS
/* ========================================================================== */

/*!
 * @brief           Handling of the ready outputs
 *
 * @param[in]       timestamp       time in nanoseconds
 * @param[in]       iaq             IAQ signal
 * @param[in]       iaq_accuracy    accuracy of IAQ signal
 * @param[in]       temperature     temperature signal
 * @param[in]       humidity        humidity signal
 * @param[in]       pressure        pressure signal
 * @param[in]       raw_temperature raw temperature signal
 * @param[in]       raw_humidity    raw humidity signal
 * @param[in]       gas             raw gas sensor signal
 * @param[in]       bsec_status     value returned by the bsec_do_steps() call
 *
 * @return          none
 */
void output_ready(int64_t timestamp, float iaq, uint8_t iaq_accuracy, float temperature, float humidity,
                  float pressure, float raw_temperature, float raw_humidity, float gas, bsec_library_return_t bsec_status)
{
    /*
    Serial.print("iaq_accuracy=");
    Serial.print(iaq_accuracy);
    Serial.print(", iaq=");
    Serial.print(iaq);
    Serial.print(", bsec_status=");
    Serial.print(bsec_status);
    */
    /**
    * Analog read value: [0; 1024]
    * iaq value:         [0; 500]
    * delta = max-min
    * offset = 150 - delta/2
    * 
    * trigger value sould be trimmed between [100; 200] => f(x) = analog * delta / 1024 + offset
    * 
    * Example 1: offset = 100, delta = 100 => f(x)=analog * x/10,24 + 100
    * Example 2: offset = 130, delta = 40  => f(x)=analog * x/25,6 + 130
    * Example 3: offset = 50,  delta = 200 => f(x)=analog * x/5,12 + 50
    */
    float delta = 100.0;
    float offset = 150.0 - delta / 2;

    float triggerValue = analogRead(POTI_PIN) * delta / 1024.0 + offset;

    Serial.print(iaq_accuracy);
    Serial.print(" ");
    Serial.print(iaq);
    Serial.print(" ");
    Serial.println(triggerValue);

    if (bsec_status == BSEC_OK)
    {
        /* status switch from 0 to 1/2/3 -> switch off blinking */
        if (iaq_accuracy == IAQ_ACCURACY_INIT)
        {
            state = STATE_RAMPUP;
        }
        else
        {
            if (iaq < triggerValue)
            {
                state = STATE_OK;
            }
            else
            {
                state = STATE_BAD;
            }
        }
    }
    else
    {
        state = STATE_UNDEFINED;
    }
    /* send the data to a server or file or where ever */
    sendData(timestamp, iaq, iaq_accuracy, temperature, humidity, pressure, triggerValue);

    /* switch on/off LEDs */
    handle_led();
}

/*! @}*/
