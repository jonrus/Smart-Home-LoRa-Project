#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"

//Ticker
#include <Ticker.h>

//LoRa
#include <SPI.h>
#include <LoRa.h>

//OLED
#include <Wire.h>

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

//Other Pins
#define BATT_ADC_PIN 13
#define BATT_RELAY_PIN 12

//Globals
byte localAdr = ShedRelayAddress; // Easy way to refer to ourself
LoRaData sendMsg;
LoRaData recvMsg;
Ticker voltCheckRelayTicker;

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
void sendLoRaMsg() {
    //*Reminder we're using a pointer here.... Spending too long with Python/JS
    LoRa.beginPacket();
    LoRa.write(sendMsg.destAdr);
    LoRa.write(localAdr);
    LoRa.write(sendMsg.ID);
    LoRa.write(sendMsg.relayState);
    LoRa.write(sendMsg.message.length());
    LoRa.print(sendMsg.message);
    LoRa.endPacket();

    sendMsg.ID++; //! Should we do this here or at the ack???
    LoRa.receive();   //Return to listen mode
}
void sendRelayMessage(bool relayState) {
    if (relayState) {
        //Relay is turning on/closing
    }
    else {
        //Relay is turning off/opening
    }
}
float readBatteryVoltage() {
    float adc = analogRead(BATT_ADC_PIN);
    float volts = (adc * 3.52) / (4095.0);  //Custom tune the ref voltage value
    volts = volts / 0.2;
    return volts;
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

        voltCheckRelayTicker.detach(); //Unsure if I need to detach first but why not
        voltCheckRelayTicker.attach(20, checkBatteryVoltage); //TODO Use 900 for 15 minutes

        //Send message
        sendRelayMessage(false);
    }
    else {
        if (currentBattVoltage <= TurnRelayOnVolts && lastBattVoltage <= TurnRelayOnVolts) {
            // Time to turn the relay on
            digitalWrite(BATT_RELAY_PIN, HIGH);
            relayOn = true;
            lastBattVoltage = 99.99; //Reset so next loop we have to do the double check again

            //Update ticker
            voltCheckRelayTicker.detach();
            voltCheckRelayTicker.attach(10, checkBatteryVoltage); //TODO use 7200 for 120 minutes

            //Send message
            sendRelayMessage(true);
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

    //Setup Lora as a callback so we can be duplex
    LoRa.onReceive(onLoRaReceive);
    LoRa.receive();
}
void initStruct(LoRaData &l) {
    l.destAdr = 0x00;
    l.ID = 0x00;
    l.Length = 0x00;
    l.message = "";
    l.reciverAdr = 0x00;
    l.RSSI = 0;
    l.senderAdr = 0x00;
    l.SNR = 0.00;
    l.relayState = 0x00;  //0x00 = open OR 0xFF closed
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
///////////////////////////
// Setup/Loop
///////////////////////////
void setup() {
    //Set up Serial
    Serial.begin(9600);
    Serial.println("Base Station Starting");
    
    setUpPins();
    setUpOLED();
    setUpLoRa();
    setUpStructs();


    //Attach Tickers
    voltCheckRelayTicker.attach(10, checkBatteryVoltage);  //TODO Use 900 for 15 minutes
}

void loop() {

}