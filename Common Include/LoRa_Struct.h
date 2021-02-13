//This is a struct to work with our unique LoRa data
struct LoRaData {
    String message;
    int reciverAdr;
    uint8_t senderAdr;  //! Unused?
    uint8_t ID;
    uint8_t Length;
    uint8_t destAdr;
    uint8_t relayState; //* 0x00 = open OR 0xFF closed
    uint8_t tempWhole;
    uint8_t tempDec;
    uint8_t humWhole;
    uint8_t humDec;
    uint8_t battWhole;
    uint8_t battDec;
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
