//This is a struct to work with our unique LoRa data
struct LoRaData {
    String message = "";
    int reciverAdr = -1;
    uint8_t senderAdr = 0x00;  //! Unused?
    uint8_t ID = 0x00;
    uint8_t Length = 0x00;
    uint8_t destAdr = 0x00;
    uint8_t relayState = 0x00; //* 0x00 = open OR 0xFF closed
    uint8_t tempWhole = 0x00;
    uint8_t tempDec = 0x00;
    uint8_t humWhole = 0x00;
    uint8_t humDec = 0x00;
    uint8_t battWhole = 0x00;
    uint8_t battDec = 0x00;
    int RSSI = 0;
    float SNR = 0.0;
};