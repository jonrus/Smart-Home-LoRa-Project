//This is a struct to work with our unique LoRa data
struct LoRaData {
    String message;
    int reciverAdr;
    byte senderAdr;  //! Unused?
    byte ID;
    byte Length;
    byte destAdr;
    int RSSI;
    float SNR;
};