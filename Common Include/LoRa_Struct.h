//This is a struct to work with our unique LoRa data
struct LoRaData {
    String message;
    int reciverAdr;
    byte senderAdr;  //! Unused?
    byte ID;
    byte Length;
    byte destAdr;
    byte relayState; //* 0x00 = open OR 0xFF closed
    byte tempWhole;
    byte tempDec;
    byte humWhole;
    byte humDec;
    byte battWhole;
    byte battDec;
    int RSSI;
    float SNR;
};

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
    l.tempWhole = 0x00;
    l.tempDec = 0x00;
    l.humWhole = 0x00;
    l.humDec = 0x00;
    l.battWhole = 0x00;
    l.battDec = 0x00;
}