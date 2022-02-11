/** =========================================================================
 * @file main.cpp
 * @brief AtlasScientific sensor data logger        
 * @author Jake Jones <jake@oakst.io.>
 * @note Adapted from simple_data_logger by Sara Geleskie Damiano 
 * @note Build Environment: Visual Studios Code with PlatformIO
 * @note Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
 *
 * DISCLAIMER:
 * THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
 * ======================================================================= */

// ==========================================================================
//  Essential includes
// ==========================================================================
// The Arduino library is needed for every Arduino program.
#include <Arduino.h>
// EnableInterrupt is used by ModularSensors for external and pin change
// interrupts and must be explicitly included in the main program.
#include <EnableInterrupt.h>
// Include the main header for ModularSensors
#include <ModularSensors.h>

// ==========================================================================
//  Data Logging Options
// ==========================================================================
// The name of this program file
const char *sketchName = "main.cpp";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char *LoggerID = "0001";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 5;
// Your logger's timezone.
const int8_t timeZone = -6;  // Central Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!
// Set the input and output pins for the logger
// NOTE:  Use -1 for pins that do not apply
const int32_t serialBaud = 115200;  // Baud rate for debugging
const int8_t greenLED = 8;          // Pin for the green LED
const int8_t redLED = 9;            // Pin for the red LED
const int8_t buttonPin = 21;        // Pin for debugging mode (ie, button pin)
const int8_t wakePin = 31;          // MCU interrupt/alarm pin to wake from sleep
// Set the wake pin to -1 if you do not want the main processor to sleep.
const int8_t sdCardPwrPin = -1;    // MCU SD card power pin
const int8_t sdCardSSPin = 12;     // SD card chip select/slave select pin
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power

// ==========================================================================
//  Modem Setup
// ==========================================================================

#include <modems/DigiXBeeLTEBypass.h>
HardwareSerial &modemSerial = Serial1;  // Use hardware serial if possible
const int32_t modemBaud = 9600;         // All XBee's use 9600 by default
// Modem Pins - Describe the physical pin connection of your modem to your board
// NOTE:  Use -1 for pins that do not apply
// The pin numbers here are for a Digi XBee with a Mayfly and LTE adapter
// For options https://github.com/EnviroDIY/LTEbee-Adapter/edit/master/README.md
const int8_t modemVccPin = 18;     // MCU pin controlling modem power
                                   // Option: modemVccPin = A5, if Mayfly SJ7 is
                                   // connected to the ASSOC pin
const int8_t modemStatusPin = 19;  // MCU pin used to read modem status
// NOTE:  If possible, use the `STATUS/SLEEP_not` (XBee pin 13) for status, but
// the CTS pin can also be used if necessary
const bool useCTSforStatus = true;  // Flag to use the CTS pin for status
const int8_t modemResetPin = A5;    // MCU pin connected to modem reset pin
const int8_t modemSleepRqPin = 23;  // MCU pin for modem sleep/wake request
const int8_t modemLEDPin = redLED;  // MCU pxin connected an LED to show modem
                                    // status

const char *apn = "hologram";  // The APN for the gprs connection

DigiXBeeLTEBypass modemXBLTEB(&modemSerial, modemVccPin, modemStatusPin,
                              useCTSforStatus, modemResetPin, modemSleepRqPin,
                              apn);
// Create an extra reference to the modem by a generic name
DigiXBeeLTEBypass modem = modemXBLTEB;

// ==========================================================================
//  Sensor Setup
// ==========================================================================

// ================
// Processor sensor
// ================
#include <sensors/ProcessorStats.h>
// Create the main processor chip "sensor" - for general metadata
const char *mcuBoardVersion = "v1.0";
ProcessorStats mcuBoard(mcuBoardVersion);
Variable *processorSampleNum = new ProcessorStats_SampleNumber(&mcuBoard);
Variable *processorFreeRAM = new ProcessorStats_FreeRam(&mcuBoard);
Variable *processorBattery = new ProcessorStats_Battery(&mcuBoard );

// ================
// Modem sensor
// ================
Variable *modemRSSI = new Modem_RSSI(&modem);
Variable *modemSignalPct = new Modem_SignalPercent(&modem);
Variable *modemBatteryState = new Modem_BatteryState(&modem);
Variable *modemBatteryPct = new Modem_BatteryPercent(&modem);
Variable *modemBatteryVoltage = new Modem_BatteryVoltage(&modem);
Variable *modemTemperature = new Modem_Temp(&modem);

// ================
// Maxim DS3231 RTC
// ================
#include <sensors/MaximDS3231.h>
MaximDS3231 ds3231(1);
Variable *rtcTemp = new MaximDS3231_Temp(&ds3231);

// =====================
//  AtlasScientific RTD
// ======================
#include <sensors/AtlasScientificRTD.h>
const int8_t AtlasRTDPower = -1;  // Power pin (-1 if unconnected)
AtlasScientificRTD atlasRTD(AtlasRTDPower);
Variable *atlasTemp = new AtlasScientificRTD_Temp(&atlasRTD);

// ==================
// AtlasScientific EC
// ===================
#include <sensors/AtlasScientificEC.h>
const int8_t AtlasECPower = -1;  // Power pin (-1 if unconnected)
AtlasScientificEC atlasEC(AtlasECPower);
// Create four variable pointers for the EZO-ES
Variable *atlasCond = new AtlasScientificEC_Cond(&atlasEC);
Variable *atlasTDS = new AtlasScientificEC_TDS(&atlasEC);
Variable *atlasSal = new AtlasScientificEC_Salinity(&atlasEC);
Variable *atlasGrav = new AtlasScientificEC_SpecificGravity(&atlasEC);

// Calculate the specific EC
float calculateAtlasSpCond(void) {
    float spCond = -9999;  // Always safest to start with a bad value
    float waterTemp = atlasTemp->getValue();
    float rawCond = atlasCond->getValue();
    // ^^ Linearized temperature correction coefficient per degrees Celsius.
    // The value of 0.019 comes from measurements reported here:
    // Hayashi M. Temperature-electrical conductivity relation of water for
    // environmental monitoring and geophysical data inversion. Environ Monit
    // Assess. 2004 Aug-Sep;96(1-3):119-28.
    // doi: 10.1023/b:emas.0000031719.83065.68. PMID: 15327152.
    if (waterTemp != -9999 && rawCond != -9999) {
        // make sure both inputs are good
        float temperatureCoef = 0.019;
        spCond = rawCond / (1 + temperatureCoef * (waterTemp - 25.0));
    }
    return spCond;
}

// Properties of the calculated variable
// The number of digits after the decimal place
const uint8_t atlasSpCondResolution = 0;
// This must be a value from http://vocabulary.odm2.org/variablename/
const char *atlasSpCondName = "specificConductance";
// This must be a value from http://vocabulary.odm2.org/units/
const char *atlasSpCondUnit = "microsiemenPerCentimeter";
// A short code for the variable
const char *atlasSpCondCode = "atlasSpCond";
// Finally, create the specific conductance variable and return a pointer to it
Variable *atlasSpCond =
    new Variable(calculateAtlasSpCond, atlasSpCondResolution, atlasSpCondName,
                 atlasSpCondUnit, atlasSpCondCode);

// ==================
// AtlasScientific pH
// ===================
#include <sensors/AtlasScientificpH.h>
const int8_t AtlaspHPower = -1;  // Power pin (-1 if unconnected)
AtlasScientificpH atlaspH(AtlaspHPower);
Variable *atlaspHpH = new AtlasScientificpH_pH(&atlaspH);

// ==================
// AtlasScientific DO
// ==================
#include <sensors/AtlasScientificDO.h>
const int8_t AtlasDOPower = -1;  // Power pin (-1 if unconnected)
AtlasScientificDO atlasDO(AtlasDOPower);
Variable *atlasDOconc = new AtlasScientificDO_DOmgL(&atlasDO);
Variable *atlasDOpct = new AtlasScientificDO_DOpct(&atlasDO);

// ==========================================================================
//   Logger setup
// ==========================================================================
Variable *variableList[] = {
    processorSampleNum,
    processorFreeRAM,
    processorBattery,
    rtcTemp,
    modemRSSI,
    //modemSignalPct,
    //modemBatteryState,
    //modemBatteryVoltage,
    //modemBatteryPct,
    //modemTemperature,
    atlasTemp,
    atlasCond,
    //atlasSpCond,
    atlasTDS,
    atlasSal,
    atlasGrav,
    atlaspHpH,
    atlasDOconc,
    atlasDOpct};

const char *UUIDs[] =                                                      // UUID array for device sensors
{
    "c2d77237-5f4a-4fc5-85e0-b65f0c36c689",   // Sequence number (EnviroDIY_Mayfly_SampleNum)
    "9d4cb12c-083d-45da-b8f2-31f2c4b0f7eb",   // Free SRAM (EnviroDIY_Mayfly_FreeRAM)
    "2f54c561-06c9-4794-be72-ecb3c0f33a89",   // Battery voltage (EnviroDIY_Mayfly_Batt)
    "b43ff7e0-8831-4ecc-a4b8-c5cee1f6b266",   // Temperature (Maxim_DS3231_Temp)
    "2a20259b-9935-4e86-928b-259778377d75",   // Received signal strength indication (Digi_Cellular_RSSI)
    "b0d1cb75-3442-460a-8b9c-471fd657e53f",   // Temperature (Atlas_Temp)
    "b6372e58-e280-4d6a-a15d-e4cd6215f2b7",   // Electrical conductivity (Atlas_Conductivity)
    "714bdd0d-02bf-4e62-a389-4be63cf7b7ec",   // Solids, total dissolved (Atlas_TDS)
    "930c90bb-5ffe-49ee-b50d-5246f5ebd52b",   // Salinity (Atlas_Salinity)
    "7e44217d-527d-4020-a774-3115c2c27a0e",   // Gage height (Atlas_SpecificGravity)
    "9bd85870-801d-4b8a-8275-5c47cbe5d794",   // pH (Atlas_pH)
    "be51f2d0-7406-43d0-8fd3-d6ca9cab55ec",   // Oxygen, dissolved (Atlas_DOconc)
    "6bdb35da-26f5-4d22-a623-ff2e6fef32a8"    // Oxygen, dissolved percent of saturation (Atlas_DOpct)
};



// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);
VariableArray varArray(variableCount, variableList, UUIDs);
Logger dataLogger(LoggerID, loggingInterval, &varArray);

// ==========================================================================
//  Data Publisher Setup
// ==========================================================================
// Device registration and sampling feature information can be obtained after
// registration at https://monitormywatershed.org or https://data.envirodiy.org
const char *registrationToken =
    "c03aa02d-7a06-4c5b-9d97-1829b7ec0b92";  // Device registration token
const char *samplingFeature =
    "797d8c01-904e-4468-a327-599806509fd6";  // Sampling feature UUID

// Create a data publisher for the Monitor My Watershed/EnviroDIY POST endpoint
#include <publishers/EnviroDIYPublisher.h>
EnviroDIYPublisher EnviroDIYPOST(dataLogger, &modem.gsmClient,
                                 registrationToken, samplingFeature);

// ==========================================================================
//  Other functions
// ==========================================================================
// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75) {
    for (uint8_t i = 0; i < numFlash; i++) {
        digitalWrite(greenLED, HIGH);
        digitalWrite(redLED, LOW);
        delay(rate);
        digitalWrite(greenLED, LOW);
        digitalWrite(redLED, HIGH);
        delay(rate);
    }
    digitalWrite(redLED, LOW);
}

float getBatteryVoltage() {
    if (mcuBoard.sensorValues[0] == -9999)
        mcuBoard.update();
    return mcuBoard.sensorValues[0];
}

// ==========================================================================
//  Main setup
// ==========================================================================
void setup() {
    // Start the primary serial connection
    Serial.begin(serialBaud);

    // Print a start-up note to the first serial port
    Serial.print(F("Now running "));
    Serial.print(sketchName);
    Serial.print(F(" on Logger "));
    Serial.println(LoggerID);
    Serial.println();

    Serial.print(F("Using ModularSensors Library version "));
    Serial.println(MODULAR_SENSORS_VERSION);
    Serial.print(F("TinyGSM Library version "));
    Serial.println(TINYGSM_VERSION);
    Serial.println();

    // Start the serial connection with the modem
    modemSerial.begin(modemBaud);

    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();

    // Set the timezones for the logger/data and the RTC
    // Logging in the given time zone
    Logger::setLoggerTimeZone(timeZone);
    // It is STRONGLY RECOMMENDED that you set the RTC to be in UTC (UTC+0)
    Logger::setRTCTimeZone(0);

    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modem);
    modem.setModemLED(modemLEDPin);
    // modem.modemWake();
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sdCardPwrPin, buttonPin,
                             greenLED);

    // Begin the variable array[s], logger[s], and publisher[s]
    dataLogger.begin();

    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up sensors..."));
        varArray.setupSensors();
    }

    // Sync the clock if it isn't valid or we have battery to spare
    if (getBatteryVoltage() > 3.55 || !dataLogger.isRTCSane()) {
        // Synchronize the RTC with NIST
        // This will also set up the modem
        dataLogger.syncRTC();
    }

    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4) {
        Serial.println(F("Setting up file on SD card"));
        dataLogger.turnOnSDcard(
            true);                       // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(
            true);  // true = wait for internal housekeeping after write
    }

    // Call the processor sleep
    Serial.println(F("Putting processor to sleep\n"));
    dataLogger.systemSleep();
}

// ==========================================================================
//  Main loop
// ==========================================================================
void loop() {
    if (getBatteryVoltage() < 3.4) {
        dataLogger.systemSleep();
    }
    // At moderate voltage, log data but don't send it over the modem
    else if (getBatteryVoltage() < 3.55) {
        dataLogger.logData();
    }
    // If the battery is good, send the data to the world
    else {
        dataLogger.logDataAndPublish();
    }
}