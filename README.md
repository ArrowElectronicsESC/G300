# G300
DLink G300 Project

# LED Colors
As part of the boot sequence of the G300, the status LED will change colors many times. Near the end of the boot sequence, the status LED will turn red and after 30 seconds, the demo will run. Once the demo is running the colors can be decoded as follows:
  - Red -> Green -> Yellow: When the demo starts it will quickly cycle through these colors to indicate the demo has started
  - Flashing Yellow: Waiting for internet connection
  - Flashing Yellow and Green: Searching for Thunderboard BLE device
  - Flashing Green: Connection Established with Thunderboard
  - Flashing Green and Red: Reading Sensor Values
  - Yellow for one second -> Green half second -> Red half second: When new sensor values are ready to be sent to azure, the LED will turn yellow. When the values of been sent, the LED will flash green then red.
  - Flashing Red: A Fatal error has occured and the program must stop.