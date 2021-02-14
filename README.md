## About

A simple Arduino project, using Arduino and ESP32 devies. It provides simple monitoring and WiFi to several devices.

This project will be unique, to my home and situation. Not much will apply to other projects or use cases.

### Devices

- Base Station

  - ESP32 (TTGO LoRa) + 433 MHz receiver
    - Provides LoRa to WiFi bridging for client devices
    - Touch input - to wake OLED
    - LEDs
      - Shed Relay state
      - Filament Monitor battery [low] state
    - Enclosure
      - 3D printed case

- Shed Relay

  - ESP32 (TTGO LoRa)
    - Monitors
      - Solar Panel output
      - Battery Voltage
      - Environment (BME280)
        - Temperature
        - Humidity
        - Air Pressure - (Not currently reported to base station)
    - Controls Relay based on Battery voltage
      - Battery powers
        - Shed lighting
        - [PiAware](https://flightaware.com/adsb/piaware/build)
      - Relay connects mains powered battery charger to battery.
    - Reports (via LoRa) to Base Station

- Filament Monitor

  - Arduino Pro Mini (3.3v)
    - Monitors
      - Environment (BME280)
        - Temp
        - Humidity
        - ~~Air Pressure (?)~~
      - Battery Voltage (of self)
    - Reports to Base Station
      - ~~via 433 mhz~~
      - via 2.4 Ghz (NRF24L01)
  
  