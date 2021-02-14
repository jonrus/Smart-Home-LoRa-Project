#include <Arduino.h>

//NRF24L01 Raido
#include <RH_NRF24.h>

//BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME_ADDR 0x76

//Real Time Clock
#include <DS3232RTC.h>

//AVR/Sleep
#include <avr/sleep.h>

//Pins
#define WAKE_PIN 2 //Pin the RTC to interrupt to wake the device
#define BATT_ADC_PIN A0

// Globals
RH_NRF24 nrf24(9,10);
Adafruit_BME280 bme;
float humd;
float temp;
float batt;
///////////////////////////
// Functions - General
///////////////////////////
void wakeUp() {
    //short and sweet
    sleep_disable();
    detachInterrupt(0);
}
void goToSleep() {
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp, LOW); //0 is interrupt on pin "2"
    delay(200);
    sleep_cpu();

    //* Code resumes here after wake up

    //Clear alarms
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
}
time_t compileTime() {
    // function to return the compile date and time as a time_t value
    //Direct from lib examples
    const time_t FUDGE(10);    //fudge factor to allow for upload time, etc. (seconds, YMMV)
    const char *compDate = __DATE__, *compTime = __TIME__, *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char compMon[4], *m;

    strncpy(compMon, compDate, 3);
    compMon[3] = '\0';
    m = strstr(months, compMon);

    tmElements_t tm;
    tm.Month = ((m - months) / 3 + 1);
    tm.Day = atoi(compDate + 4);
    tm.Year = atoi(compDate + 7) - 1970;
    tm.Hour = atoi(compTime);
    tm.Minute = atoi(compTime + 3);
    tm.Second = atoi(compTime + 6);

    time_t t = makeTime(tm);
    return t + FUDGE;        //add fudge factor to allow for compile time
}
void setUpRTC () {
    RTC.begin();
    // initialize the alarms to known values, clear the alarm flags, clear the alarm interrupt flags
    RTC.setAlarm(ALM1_MATCH_DATE, 0, 0, 0, 1);
    RTC.setAlarm(ALM2_MATCH_DATE, 0, 0, 0, 1);
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    RTC.alarmInterrupt(ALARM_1, false);
    RTC.alarmInterrupt(ALARM_2, false);
    RTC.squareWave(SQWAVE_NONE);

    // set the RTC time and date to the compile time
    RTC.set(compileTime());
    setSyncProvider(RTC.get);   // the function to get the time from the RTC
    if(timeStatus() != timeSet) {
        Serial.println("Unable to sync with the RTC");
    }

    // set Alarm 1 to occur at 5 seconds after every minute
    RTC.setAlarm(ALM1_MATCH_SECONDS, 5, 0, 0, 1);   //TODO Alarm at 15 after every hour
    RTC.setAlarm(ALM2_MATCH_HOURS, 45, 0, 0);       //Alarm at 45 after every hour
    // clear the alarm flag
    RTC.alarm(ALARM_1);
    RTC.alarm(ALARM_2);
    // set Alarm to interrupt
    RTC.alarmInterrupt(ALARM_1, true);
}
void setUpRadio() {
    if (!nrf24.init()) {
        Serial.println("Radio init failed");
        goToSleep();    //Goto sleep and hope things work out better next time
    }
    if (!nrf24.setChannel(1)) {
        Serial.println("Radio channel set failed");
    }
    if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)) {
        Serial.println("Radio setRF failed");
    }
}
void setUpBME() {
    if (!bme.begin(BME_ADDR)) {  
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        goToSleep();    //Goto sleep and hope things work out better next time
    }
    //refer to below for info
    //https://github.com/adafruit/Adafruit_BME280_Library/blob/master/examples/advancedsettings/advancedsettings.ino
    //http://adafruit.github.io/Adafruit_BME280_Library/html/class_adafruit___b_m_e280.html
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X2,   //Temperature
                    Adafruit_BME280::SAMPLING_NONE, //Pressure - Not used in this project
                    Adafruit_BME280::SAMPLING_X2,   //Humidity
                    Adafruit_BME280::FILTER_X2);

}
void setUpPins() {
    pinMode(WAKE_PIN, INPUT_PULLUP);
}
void readBatteryVoltage() {
    float adc = analogRead(BATT_ADC_PIN);
    float volts = (adc * 3.3) / (1024.0);  //Custom tune the ref voltage here
    batt = volts / 0.2;  //0.2 is the the result of  (R2/(R1+R2)) of our voltage divider
}
void readBME() {
    //Have to force the measurement due to our settings
    bme.takeForcedMeasurement();
    temp = (bme.readTemperature() * 1.8) + 32;  //Convert from C to F
    humd = bme.readHumidity();
}
void sleepBME(int addr) {
    //Should reduce power draw
    //Info from: https://www.youtube.com/watch?v=O8FgrHR2laM
    Wire.beginTransmission(addr);
    Wire.write((uint8_t)0xF4);
    Wire.write((uint8_t)0b00000000);
    Wire.endTransmission();
}
void sendNRF24Data() {
    // Create a string with our values and a | as the separator
    String message = String(temp, 2) + "|" + String(humd, 2) + "|" + String(batt, 2);
    unsigned int messageLen = message.length();
    char messageBuffer[40];
    message.toCharArray(messageBuffer, 40);

    nrf24.send((uint8_t *)messageBuffer, messageLen);
    nrf24.waitPacketSent();

    Serial.println(message);

    if (nrf24.waitAvailableTimeout(500)) {
        //Should get an ack from the server
        uint8_t resBuf[RH_NRF24_MAX_MESSAGE_LEN];
        uint8_t resLen = sizeof(resBuf);

        if (nrf24.recv(resBuf, &resLen)) {
            Serial.println("Got a reply");
        }
        else {
            Serial.println("send/recv failed");
        }
    }
    else {
        Serial.println("No reply from Base Station");
    }
}

// Loop
void setup()
{
    Serial.begin(9600);
    Serial.println("$");
    setUpRTC();
    setUpBME();
    setUpRadio();
}
void loop()
{   
    delay(2000);    //Give time for the BME to warm up
    readBME();
    readBatteryVoltage();
    delay(500);
    sendNRF24Data();
    sleepBME(BME_ADDR);

    //Everything is done, go to sleep
    goToSleep();
}
