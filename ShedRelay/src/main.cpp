#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"

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
#define BAT_ADC_PIN 13

//Globals
byte localAdr = ShedRelayAddress; // Easy way to refer to ourself
LoRaData sentMsg;
LoRaData recvMsg;

///////////////////////////
// Functions - General
///////////////////////////
void myLoRaReceive(int inPacket, LoRaData &msg) {
    //Check for packet and return if nothing
    if (inPacket == 0) return;
    
    //Read data and populate the struct
    msg.reciverAdr = LoRa.read();   //Int becuase can be -1 if no data
    msg.senderAdr = LoRa.read();
    msg.ID = LoRa.read();
    msg.Length = LoRa.read();
    //TODO Added rest of our custom bytes
    msg.RSSI = LoRa.rssi();
    msg.SNR = LoRa.packetSnr();

    //Clear out any eariler message
    msg.message = "";

    //Work over remaining data
    while (LoRa.available()) {
      msg.message += (char)LoRa.read();
    }

    //Make sure our message was as long as it should be
    if (msg.Length != msg.message.length()) {
      //TODO Add broadcast for resend
      return;  // It wasn't so return without acting on data
    }

    //Make sure this message was sent for this unit OR all units
    if (msg.reciverAdr != localAdr && msg.reciverAdr != BroadcastAddress) {
      //TODO Add error/serial print/or whatever
      return;
    }

    //Message must be for this unit
    //TODO
}

void onLoRaReceive(int pSize) {
  // Just using this to call our own function
  myLoRaReceive(pSize, recvMsg);
}
void sendLoRaMsg(LoRaData &msg) {
    //*Reminder we're using a pointer here.... Spending too long with Python/JS
    LoRa.beginPacket();
    LoRa.write(msg.destAdr);
    LoRa.write(localAdr);
    LoRa.write(msg.ID);
    LoRa.write(msg.message.length());
    LoRa.print(msg.message);
    LoRa.endPacket();

    msg.ID++; //! Should we do this here or at the ack???
    LoRa.receive();   //Return to listen mode
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
}
void setUpStructs() {
    //Put blank/starting data in all the strucs
    initStruct(recvMsg);
    initStruct(sentMsg);

    //Unique settings Here
    sentMsg.destAdr = BaseStationAddress;
    sentMsg.senderAdr = localAdr;
}
void setUpPins() {

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
}

void loop() {
  float adc = analogRead(BAT_ADC_PIN);
  float voltage = (adc * 3.52) / (4095.0);  //Custom tune the ref voltage value
  voltage = voltage / 0.2;
  sentMsg.message = voltage;
  sendLoRaMsg(sentMsg);
  delay(1000);
}