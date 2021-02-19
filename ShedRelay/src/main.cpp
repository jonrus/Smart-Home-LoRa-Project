#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"

//Ticker
#include <Ticker.h>

//LoRa
#include <SPI.h>
#include <LoRa.h>

//BME280
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

//LoRa Pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18     // Chip Select
#define RST 14    // Chip Reset
#define DIO0 26   // Chip Irq
#define BAND 915E6//915E6 for North America

//OLED pins - Disabled on the shed so just use rest to turn it off
#define OLED_RST 16

//BME280 Pins (General I2C)
#define BME_SDA_PIN 21
#define BME_SCL_PIN 22

//Relay Pins
#define BATT_ADC_PIN 13
#define BATT_RELAY_PIN 12

//Globals
byte localAdr = ShedRelayAddress; // Easy way to refer to ourself
LoRaData sendMsg;
LoRaData recvMsg;
Ticker voltCheckRelayTicker;
Ticker sendStatusUpdateTicker;
Adafruit_BME280 bme;

//Battery Voltage Defines
#define TurnRelayOnVolts 11.9
#define TurnRelayOffVolts 12.4
///////////////////////////
// Functions - General
///////////////////////////
void onLoRaReceive(int inPacket) {
    //Check for packet and return if nothing
    if (inPacket == 0) return;
    
    //Read data and populate the struct
    recvMsg.reciverAdr = LoRa.read();   //Int becuase can be -1 if no data
    recvMsg.senderAdr = LoRa.read();
    recvMsg.ID = LoRa.read();
    recvMsg.relayState = LoRa.read();
    recvMsg.tempWhole = LoRa.read();
    recvMsg.tempDec = LoRa.read();
    recvMsg.humWhole = LoRa.read();
    recvMsg.humDec = LoRa.read();
    recvMsg.battWhole = LoRa.read();
    recvMsg.battDec = LoRa.read();
    //TODO Added rest of our custom bytes
    recvMsg.Length = LoRa.read();
    recvMsg.RSSI = LoRa.rssi();
    recvMsg.SNR = LoRa.packetSnr();

    //Clear out any eariler message
    recvMsg.message = "";

    //Work over remaining data
    while (LoRa.available()) {
      recvMsg.message += (char)LoRa.read();
    }

    //Make sure our message was as long as it should be
    if (recvMsg.Length != recvMsg.message.length()) {
      //TODO Add broadcast for resend
      return;  // It wasn't so return without acting on data
    }

    //Make sure this message was sent for this unit OR all units
    if (recvMsg.reciverAdr != localAdr && recvMsg.reciverAdr != BroadcastAddress) {
      //TODO Add error/serial print/or whatever
      return;
    }

    //Message must be for this unit
    //TODO
}
void sendLoRaMsg(LoRaData &msg) {
    //*Reminder we're using a pointer here.... Spending too long with Python/JS
    LoRa.beginPacket();
    LoRa.write(msg.destAdr);
    LoRa.write(localAdr);
    LoRa.write(msg.ID);
    LoRa.write(msg.relayState);
    LoRa.write(msg.tempWhole);
    LoRa.write(msg.tempDec);
    LoRa.write(msg.humWhole);
    LoRa.write(msg.humDec);
    LoRa.write(msg.battWhole);
    LoRa.write(msg.battDec);
    LoRa.write(msg.message.length());
    LoRa.print(msg.message);
    LoRa.endPacket();

    msg.ID++; //! Should we do this here or at the ack???
    LoRa.receive();   //Return to listen mode

    Serial.println("msg sent");
}
void splitNumOnDec(float inNum, byte &outWhole, byte &outDec) {
    byte wholeNum;
    byte decNum;

    wholeNum = inNum;
    decNum = inNum * 10 - wholeNum * 10;

    outWhole = wholeNum;
    outDec = decNum;
}
float readBatteryVoltage() {
    float adc = analogRead(BATT_ADC_PIN);
    float volts = (adc * 3.52) / (4095.0);  //Custom tune the ref voltage value
    volts = volts / 0.2;  //0.2 is the the result of  (R2/(R1+R2)) of our voltage divider
    return volts;
}
void sendStatusMessage() {
    //Update needed values before calling sendLoRaMsg()
    float temp = (bme.readTemperature() * 1.8) + 32;
    float hum = bme.readHumidity();
    float batVolts = readBatteryVoltage();

    splitNumOnDec(temp, sendMsg.tempWhole, sendMsg.tempDec);
    splitNumOnDec(hum, sendMsg.humWhole, sendMsg.humDec);
    splitNumOnDec(batVolts, sendMsg.battWhole, sendMsg.battDec);

    sendLoRaMsg(sendMsg);
}
void checkBatteryVoltage() { //*Called by Ticker
    /*
      This function is called by a ticker.
      When the relay is open/off it is called every 15 minutes.
      When the relay is on/closed it is called once every 120 minutes.
          The battery charger I have will go into trickle mode if batt voltage is good

    */
    static float lastBattVoltage = 99.99;  //Start with something high so we don't turn the relay on first call
    static bool relayOn = false;
    float currentBattVoltage = readBatteryVoltage();

    if (relayOn) {
        digitalWrite(BATT_RELAY_PIN, LOW);  //*Turn off relay
        relayOn = false;
        sendMsg.relayState = 0x00;

        voltCheckRelayTicker.detach(); //Unsure if I need to detach first but why not
        voltCheckRelayTicker.attach(20, checkBatteryVoltage); //TODO Use 900 for 15 minutes

        //Send message
        sendStatusMessage();
    }
    else {
        if (currentBattVoltage <= TurnRelayOnVolts && lastBattVoltage <= TurnRelayOnVolts) {
            // Time to turn the relay on
            digitalWrite(BATT_RELAY_PIN, HIGH);
            relayOn = true;
            sendMsg.relayState = 0xFF;
            lastBattVoltage = 99.99; //Reset so next loop we have to do the double check again

            //Update ticker
            voltCheckRelayTicker.detach();
            voltCheckRelayTicker.attach(10, checkBatteryVoltage); //TODO use 7200 for 120 minutes

            //Send message
            sendStatusMessage();
        }
        else {
            //Need to double check the volts
            lastBattVoltage = currentBattVoltage;
        }
    }
}
///////////////////////////
// Functions - Setup
///////////////////////////
void setUpOLED() {
    //reset OLED display via software
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
}
void setUpLoRa() {
    //SPI LoRa pins
    SPI.begin(SCK, MISO, MOSI, SS);
    //setup LoRa transceiver module
    LoRa.setPins(SS, RST, DIO0);

    if (!LoRa.begin(BAND)) {
      Serial.println("Starting LoRa failed!");
      while(1); // Don't proceed, loop forever
    }
    
    //Set Sync Word
    LoRa.setSyncWord(LORA_SYNC_WORD);

    //Setup Lora as a callback so we can be duplex
    LoRa.onReceive(onLoRaReceive);
    LoRa.receive();
}
void setUpStructs() {
    //Put blank/starting data in all the strucs
    initStruct(recvMsg);
    initStruct(sendMsg);

    //Unique settings Here
    sendMsg.destAdr = BaseStationAddress;
    sendMsg.senderAdr = localAdr;
}
void setUpPins() {
    pinMode(BATT_RELAY_PIN, OUTPUT);
}
void setUpBME() {
    Wire.begin(BME_SDA_PIN, BME_SCL_PIN);
    while (!bme.begin(0x76, &Wire)) { //My BME sensor has this address
      Serial.println("Unable to connect to BME");
      delay(100);
    }
}
///////////////////////////
// Setup/Loop
///////////////////////////
void setup() {
    //Set up Serial
    Serial.begin(9600);
    Serial.println("Shed Relay Starting");
    
    setUpPins();
    setUpOLED();
    setUpLoRa();  //*This is blocking
    setUpStructs();
    setUpBME();  //*This is blocking

    //Attach Tickers
    voltCheckRelayTicker.attach(10, checkBatteryVoltage);  //TODO Use 900 for 15 minutes
    sendStatusUpdateTicker.attach(30, sendStatusMessage); //TODO Use 300 for 5 minutes


    //Send a status update on first boot
    sendStatusMessage();
}

void loop() {
}
