#include <Arduino.h>
#include "../../Common Include/secerts.h"
#include "../../Common Include/LoRa_Settings.h"
#include "../../Common Include/LoRa_Struct.h"
#include "../../Common Include/NRF24_Struct.h"

//LoRa
#include <SPI.h>
#include <LoRa.h>

//OLED Adafruit
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//NRF24
#include <RH_NRF24.h>

//Ticker
#include <Ticker.h>

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
#define SHED_LED_PIN 17
//Globals
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
RH_NRF24 nrf24(12,22);  //CE = pin 12, CSN = 22
byte localAdr = BaseStationAddress; // Easy way to refer to ourself
LoRaData sendMsg;
LoRaData recvMsg;
FilamentMonitorDataInNRF24 filamentMonData;
Ticker sleepOLEDTicker;
bool needOLEDUpdate = true;
bool blankOLED = false;
bool wakeUpOLED = false;
const int touchThreshold = 60;
///////////////////////////
// Functions - General
///////////////////////////
void updateLEDPins() {
    if (needOLEDUpdate) {
        if (recvMsg.relayState == 0xFF) {
            digitalWrite(SHED_LED_PIN, HIGH);
        }
        else {
            digitalWrite(SHED_LED_PIN, LOW);
        }
    }
}
void updateOLED() {
    //* At size 1 22 chars is 1 line with 0 rotation
    //* At size 1 11 chars is 1 line with 45 rotation
    //* At size 2 11 chars is 1 line with 0 rotation
    //* At size 2 5 chars is 1 line with 45 rotation
    if (blankOLED) {
        delay(100);     //Need delay? or we crash :(
        //Seems dumb to do this, as it's writing nothing over and over
        //But no pixels are active and my device does not seem to support
        //display.ssd1306_command(SSD1306_DISPLAYOFF);
        //So this is where we're at...
        display.clearDisplay();
        display.display();
        return;
    }
    if (!needOLEDUpdate) {
        delay(100);     //Need delay? or we crash :(
        return;
    }
    
    needOLEDUpdate = false;
    
    //Build the text for the display
    char relayTemp[19];
    char relayVolt[9];
    char filamentTemp[19];
    char filamentTempBuff[6];
    char filamentHumdBuff[6];
    dtostrf(filamentMonData.temp, 2, 2, filamentTempBuff);
    dtostrf(filamentMonData.humd, 2, 2, filamentHumdBuff);
    snprintf(relayTemp, sizeof(relayTemp), "T: %d.%d  H:%d.%d%%", recvMsg.tempWhole, recvMsg.tempDec, recvMsg.humWhole, recvMsg.humDec);
    snprintf(relayVolt, sizeof(relayVolt), "V: %d.%d", recvMsg.battWhole, recvMsg.battDec);
    snprintf(filamentTemp, sizeof(filamentTemp), "T: %s  H:%s%%", filamentTempBuff, filamentHumdBuff);

    // Write everything to the display
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("    --= Relay =--");
    display.setCursor(0,10);
    display.print((char*)relayVolt);
    display.setCursor(0,20);
    display.print((char*)relayTemp);
    display.setCursor(0,40);
    display.print("   --= Filament =--");
    display.setCursor(0,50);
    display.print((char*)filamentTemp);
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
    //TODO
    needOLEDUpdate = true;  //Update the OLED next loop
}
void readNRF24Data() {
    if (nrf24.available()) {
        //Looks like a message is here
        uint8_t buf[RH_NRF24_MAX_MESSAGE_LEN];
        uint8_t len = sizeof(buf);
        if (nrf24.recv(buf, &len)) {
            //Send an ack
            uint8_t ack[] = "ACK";
            nrf24.send(ack, sizeof(ack));
            nrf24.waitPacketSent();

            //Do all this here and not another function because we only need it done here. At the moment....
            //Work over the data by spliting it and then converting to float
            char *strings[10];
            char *ptr = NULL;

            uint8_t index = 0;
            ptr = strtok((char*)buf, ":|");     //Requires a list of spliters, but we're only really using |
            while (ptr != NULL) {
                strings[index] = ptr;
                index++;
                ptr = strtok(NULL, ":|");
            }
            filamentMonData.temp = atof(strings[0]);
            filamentMonData.humd = atof(strings[1]);

            needOLEDUpdate = true;  //Update the OLED next loop
        }
    }
    else {
        //Serial.println("NRF recv failed");
    }
}
void sleepOLEDScreen() {
    blankOLED = true;
    sleepOLEDTicker.detach();
}
void checkTouchWakeScreenLoop() {
   if (wakeUpOLED) {
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    // If interrupts come faster than Xms, assume it's a bounce and ignore
    //Using a long debound time because we don't want to reattach the same ticker over and over
    if (interrupt_time - last_interrupt_time > 5000)
    {
        if (blankOLED) {  // Prevent from attaching when already attached
            blankOLED = false;
            needOLEDUpdate = true;  //Need or the screen wont update until the next message in
            sleepOLEDTicker.attach(5, sleepOLEDScreen);
        }
    }
    last_interrupt_time = interrupt_time;
   }
   wakeUpOLED = false;
}
void touchWakeScreen() {  //Called via interrupt
    //Keep it short, and check if we need to do the work in the loop
    wakeUpOLED = true;
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
    //Put blank/starting data in all the strucs so it looks okay on OLED
    initStruct(recvMsg);
    initStruct(sendMsg);
    initFilamentMon_Struct(filamentMonData);

    //Unique settings Here
    sendMsg.destAdr = ShedRelayAddress;
    sendMsg.senderAdr = localAdr;
}
void setUpNRF24Radio() {
    if (!nrf24.init()) {
        Serial.println("NRF24 Init Failed");
        //TODO Add alert via oled/leds/etc
    }
    if (!nrf24.setChannel(1)) {
        Serial.println("NRF24 channel set failed");
    }
    if (!nrf24.setRF(RH_NRF24::DataRate250kbps, RH_NRF24::TransmitPower0dBm)) {
      Serial.println("NRF24 setRF failed");
    }
    Serial.println("Exit NRF24 setup");
}
void setUpPins() {
    pinMode(SHED_LED_PIN, OUTPUT);
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
    setUpNRF24Radio();
    setUpStructs();

    updateOLED();

    //Attach Tickers
    sleepOLEDTicker.attach(5, sleepOLEDScreen);

    //Attach touch Interrupt
    touchAttachInterrupt(13, touchWakeScreen, touchThreshold);
}

void loop() {
    readNRF24Data();
    updateLEDPins(); //Needs to be called before updateOLED()
    checkTouchWakeScreenLoop();
    updateOLED();
    // yield();
}