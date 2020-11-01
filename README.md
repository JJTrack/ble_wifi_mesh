# Wi-Fi Mesh Network Repository

## Quick overview

This repository contains the Wi-Fi Mesh Network code. This Mesh Network is self healing and repairing; it connects to the TCP Server to send data packets. 

## Set Up Steps
1. Open the menuconfig using
```bash
idf.py -p [your com port] menuconfig
```
2. Navigate to TCP Configuration using the arrow keys; set the IP and the Port for the TCP server
3. In the menuconfig, navigate to Example Connection Configuration; set up the Wi-Fi SSID and the Wi-Fi Password
4. Flash the ESP32 using the flashing instructions below

## prerequisites

1. You will need to have [node](https://nodejs.org) installed.
2. The esp-idf must be set up. you can follow the instructions in my course (free of charge) on the [Setting up Your Environment](https://www.learnesp32.com/2_introduction) module or follow the [official documentation](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/#step-1-set-up-the-toolchain)
3. this template is for [vscode](https://code.visualstudio.com/download) which will need to be installed
4. In VSCODE add the [c++ extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)

5. ensure tour ESP32 is plugged in and that a COM PORT is established (You may need a driver for your ESP32 dev board)

## flashing the esp32

1. in vs code, open a new terminal by pressing ctrl + \` (or pressing F1 and typing `open new terminal`)
2. type the following command

```bash
idf.py -p [your com port] flash monitor
```
