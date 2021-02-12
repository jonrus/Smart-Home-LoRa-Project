#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"

//LoRa
#include <SPI.h>
#include <LoRa.h>

//OLED
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//LoRa Pins
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18     // Chip Select
#define RST 14    // Chip Reset
#define DIO0 26   // Chip Irq
#define BAND 915E6//915E6 for North America

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//Other Pins


//Globals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
byte localAdr = BaseStationAddress; // Easy way to refer to ourself
LoRaData sendMsg;
LoRaData recvMsg;

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
    Serial.print("Packet In: ");
    Serial.println(recvMsg.message);
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
///////////////////////////
// Functions - Setup
///////////////////////////
void setUpOLED() {
    //reset OLED display via software
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    //initialize OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
      Serial.println(F("SSD1306 allocation failed"));
      while(1); // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("Base Station ");
    display.display();
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

    Serial.println("LoRa Initializing - OK!");
    display.setCursor(0,10);
    display.println("LoRa Initializing - OK!");
    display.display();
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
    sendMsg.destAdr = ShedRelayAddress;
    sendMsg.senderAdr = localAdr;
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
    //! DELETE THIS - Just for testing
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(recvMsg.message);
    display.display();
}