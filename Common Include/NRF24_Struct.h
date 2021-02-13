struct FilamentMonitorDataInNRF24 {
    float temp;
    float humd;
    float batt;
};

void initFilamentMon_Struct(FilamentMonitorDataInNRF24 &l) {
    l.temp = 0.0;
    l.humd = 0.0;
    l.batt = 0.0;
}
