#include <Arduino.h>

//NRF24L01 Raid
#include <RH_RF24.h>

//433 Radio
#include <RH_ASK.h>
#include <SPI.h> // Not actually used but needed to compile

//BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Globals
RH_ASK driver; // Defaults to pin 12 to TX
Adafruit_BME280 bme;
float temp;
float humd;
///////////////////////////
// Functions - General
///////////////////////////
void setUpRadio() {
    driver.init();
}
void setUpBME() {
    if (!bme.begin(0x76)) {  
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
    }
}
void readBME() {
    temp = (bme.readTemperature() * 1.8) + 32;
    humd = bme.readHumidity();
}
void send433Data() {
    readBME();  //Update temp/humd values

    // Create a string with our values and a | as the separator
    String message = String(temp, 2) + "|" + String(humd, 2);
    unsigned int messageLen = message.length();
    char messageBuffer[40];  //This buffer size is over kill
    message.toCharArray(messageBuffer, 40);

    driver.send((uint8_t *)messageBuffer, messageLen);
    driver.waitPacketSent();
}

// Loop
void setup()
{
    setUpBME();
    setUpRadio();
}
void loop()
{
    send433Data();
    delay(1000);
}