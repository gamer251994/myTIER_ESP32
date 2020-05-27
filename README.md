# myTIER_ESP32
Controlling myTIER eScooters with an ESP32

This project was sparked from this thread: https://mytier-forum.de/community/topic/akkustand-via-bluetooth-auslesen/

It was discovered that the myTIER Scooters accept arbitrary Bluetooth Low Energy (BLE) connections and can be controlled when in possession of the correct BT-Password. 

The Project allows to:
- Retrieve Information/Stats from the Scooter (Battery level, Speed, ride distance, total distance, lock-status, ...)
- lock/unlock the Scooter
- turn the LED-Light On/Off
