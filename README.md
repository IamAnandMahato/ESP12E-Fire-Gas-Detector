# ESP-12E Fire and Gas Detector with Automatic Water Pump

## Project Overview
An IoT-based safety prototype built using the ESP8266 ESP-12E. It monitors gas and flame sensors, activates a buzzer alarm, controls a water pump through a relay when flame is detected, and displays sensor status on a mobile-friendly web dashboard over home Wi-Fi.

## Features
- MQ-2 gas sensor monitoring
- Flame detection using a digital flame sensor
- Buzzer alarm for gas or flame detection
- Automatic water pump control through a relay
- Mobile-friendly web dashboard
- Live sensor status updates
- Wi-Fi reconnection support

## Hardware Components
- NodeMCU ESP8266 / ESP-12E
- MQ-2 gas sensor module
- Digital flame sensor module
- Three-pin buzzer module
- Relay module compatible with ESP8266 logic
- 5V DC water pump
- Suitable power supplies and connecting wires

## Pin Connections
| Component | ESP8266 Pin |
|---|---|
| MQ-2 analog output | A0 |
| Flame sensor DO | D5 |
| Buzzer signal | D6 |
| Relay input | D1 |

## Software and Technologies
- Arduino IDE
- Embedded C++ / Arduino framework
- ESP8266 Wi-Fi
- ESP8266WebServer
- HTML, CSS, and JavaScript

## How It Works
The ESP8266 reads the gas and flame sensors continuously. It activates the buzzer when gas exceeds the configured threshold or a flame is detected. The relay-controlled pump activates on flame detection. A web dashboard displays the current readings and device status to phones connected to the same Wi-Fi network.

## Setup
1. Install the ESP8266 board package in Arduino IDE.
2. Open the project sketch.
3. Enter your Wi-Fi credentials locally.
4. Check the sensor threshold and relay trigger polarity.
5. Upload the sketch to the NodeMCU.
6. Open Serial Monitor at 115200 baud to find the local IP address.
7. Open that IP address in a phone browser connected to the same Wi-Fi network.

## Safety Notice
This is an educational prototype, not a certified fire-detection or fire-suppression system. Water can worsen electrical, oil, and certain chemical fires. Use certified safety equipment for real-world protection.

## Future Improvements
- Cloud monitoring and notifications
- Historical sensor readings
- Improved fault detection
- Enclosure and power-failure monitoring
