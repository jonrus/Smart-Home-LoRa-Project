#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"

//LoRa
#include <SPI.h>
#include <LoRa.h>

//OLED Adafruit
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//433mhz
#include <RH_ASK.h>


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
#define ASK_PIN 35  //433 mhz pin

//Globals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
RH_ASK driver433(2000, ASK_PIN);
byte localAdr = BaseStationAddress; // Easy way to refer to ourself
LoRaData sendMsg;
LoRaData recvMsg;

///////////////////////////
// Functions - General
///////////////////////////
void updateOLED() {
    //* At size 1 22 chars is 1 line with 0 rotation
    //* At size 1 11 chars is 1 line with 45 rotation
    //* At size 2 11 chars is 1 line with 0 rotation
    //* At size 2 5 chars is 1 line with 45 rotation
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("    --= Relay =--");
    display.setCursor(0,10);
    display.print("V: ");
    display.print(recvMsg.battWhole);
    display.print(".");
    display.print(recvMsg.battDec);
    display.setCursor(0,20);
    display.print("T: ");
    display.print(recvMsg.tempWhole);
    display.print(".");
    display.print(recvMsg.tempDec);
    display.print("  H: ");
    display.print(recvMsg.humWhole);
    display.print(".");
    display.print(recvMsg.humDec);
    display.setCursor(0,40);
    display.print("   --= Filament =--");
    display.setCursor(0,50);
    display.print("T: 99.99  H:99.99%");
    display.display();
}
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
    updateOLED();

    //TODO
    Serial.print("Temp: ");
    Serial.print(recvMsg.tempWhole);
    Serial.print(".");
    Serial.print(recvMsg.tempDec);
    Serial.print("\tBatt: ");
    Serial.print(recvMsg.battWhole);
    Serial.print(".");
    Serial.println(recvMsg.battDec);
}
String parse433DataString(String data, char sep, int index) {

}
void read433() {
    uint8_t buff[40];
    uint8_t len = sizeof(buff);

    if (driver433.recv(buff, &len)) {
        Serial.print("433: ");
        Serial.println((char*)buff);
    }
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
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { //0x3c is built in
      Serial.println(F("SSD1306 allocation failed"));
      while(1); // Don't proceed, loop forever
    }
    display.display();
    delay(2000); //Allow OLED to start up
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
}
void setUpStructs() {
    //Put blank/starting data in all the strucs
    initStruct(recvMsg);
    initStruct(sendMsg);

    //Unique settings Here
    sendMsg.destAdr = ShedRelayAddress;
    sendMsg.senderAdr = localAdr;
}
void setUp433Radio() {
    if (!driver433.init()) {
        Serial.println("Unable to init 433 radio");
    }
}
void setUpPins() {
    pinMode(ASK_PIN, INPUT);
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
    setUp433Radio();

    updateOLED();
}

void loop() {
  read433();
}