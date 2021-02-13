#include <Arduino.h>

//NRF24L01 Raido
#include <RH_NRF24.h>

//BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Globals
RH_NRF24 nrf24(9,10);
Adafruit_BME280 bme;
float humd;
float temp;
///////////////////////////
// Functions - General
///////////////////////////
void setUpRadio() {
    if (!nrf24.init()) {
        Serial.println("Radio init failed");
        //TODO ADD go to sleep/etc
    }
    if (!nrf24.setChannel(1)) {
        Serial.println("Radio channel set failed");
    }
    if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)) {
        Serial.println("Radio setRF failed");
    }
    Serial.println("Exit Radio Setup");
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
void sendNRF24Data() {
    readBME();

    // Create a string with our values and a | as the separator
    String message = String(temp, 2) + "|" + String(humd, 2);
    unsigned int messageLen = message.length();
    char messageBuffer[40];
    message.toCharArray(messageBuffer, 40);

    nrf24.send((uint8_t *)messageBuffer, messageLen);
    nrf24.waitPacketSent();

    if (nrf24.waitAvailableTimeout(500)) {
        //Should get an ack from the server
        uint8_t resBuf[RH_NRF24_MAX_MESSAGE_LEN];
        uint8_t resLen = sizeof(resBuf);

        if (nrf24.recv(resBuf, &resLen)) {
            Serial.println("Got a reply");
            //TODO goto sleep/etc
        }
        else {
            Serial.println("send/recv failed");
        }
    }
    else {
        Serial.println("No reply from Base Station");
        //TODO goto sleep/etc
    }
}
void send433Data() {
    readBME();  //Update temp/humd values

    // Create a string with our values and a | as the separator
    String message = String(temp, 2) + "|" + String(humd, 2);
    unsigned int messageLen = message.length();
    char messageBuffer[40];
    message.toCharArray(messageBuffer, 40);

    //433 Driver
    // driver.send((uint8_t *)messageBuffer, messageLen);
    // driver.waitPacketSent();
}

// Loop
void setup()
{
    Serial.begin(9600);
    Serial.println("$");
    setUpBME();
    setUpRadio();
}
void loop()
{
    sendNRF24Data();
    delay(1000);
}
