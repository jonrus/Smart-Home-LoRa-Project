//This is a struct to work with our LoRa data
struct LoRaData {
    String message;
    int reciverAdr;
    byte senderAdr;
    byte ID;
    byte Length;
    byte destAdr;
    byte count;
    int RSSI;
    float SNR;
};