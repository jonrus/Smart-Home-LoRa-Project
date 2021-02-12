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

//Globals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
byte localAdr = BaseStationAddress; // Easy way to refer to ourself
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
    LoRa.write(msg.count);
    LoRa.write(msg.message.length());
    LoRa.print(msg.message);
    LoRa.endPacket();

    msg.count++; //! Should we do this here or at the ack???
}
///////////////////////////
// Functions - Setup
///////////////////////////
void setupOLED() {
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

///////////////////////////
// Setup/Loop
///////////////////////////
void setup() {
    //Set up Serial
    Serial.begin(9600);
    Serial.println("Base Station Starting");
    
    setupOLED();
    setUpLoRa();
}

void loop() {

}