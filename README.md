# ESP32NTP

ESP32/ESP8266 speaking pseudo GPS messages to Serial, only for time sync purpose based on NTP via WiFi.

- Implemented based on Arduino stack
- using AutoConnect(https://github.com/Hieromon/AutoConnect)
- send GPMRC messages as valid(A), not valid location or direction data.
- fixed 4800 baud, TX and TX2(ESP32 only)
- OTA support (no auth)
- mDNS enabled.

I have made this project with ESP01 for https://github.com/jose365sc/akafugu_nixie_clock.
