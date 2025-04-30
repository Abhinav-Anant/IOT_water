![CodeRabbit Pull Request Reviews](https://img.shields.io/coderabbit/prs/github/Abhinav-Anant/IOT_water?utm_source=oss&utm_medium=github&utm_campaign=Abhinav-Anant%2FIOT_water&labelColor=171717&color=FF570A&link=https%3A%2F%2Fcoderabbit.ai&label=CodeRabbit+Reviews)

# Trail Capture Camera System

## Overview
This system implements a motion-activated camera using ESP32-CAM with LD2410 radar sensor, 
SD card storage, and LoRa transmission capabilities.

## Hardware Requirements
- ESP32-CAM
- LD2410 Radar Sensor
- LoRa Module (RA-02)
- SD Card
- Power Supply (3.3V)

## Pin Connections
- Radar: RX=16, TX=17
- LoRa: SCK=18, MISO=19, MOSI=23, SS=5, RST=25, DIO0=26
- SD Card: Using VSPI interface

## Features
- Motion detection using LD2410 radar
- Image capture and storage
- LoRa transmission of captured images
- System diagnostics and error logging
- Power management with deep sleep
- Automatic SD card cleanup

## Building and Testing
1. Install required libraries:
   - ESP32 Arduino Core
   - LoRa Library
   - ESP32 Camera Driver
2. Configure settings in config.h
3. Build and upload using Arduino IDE or PlatformIO
4. Run unit tests using:
   ```
   pio test -e esp32cam
   ```

## Diagnostics
The system logs diagnostic information every minute including:
- CPU temperature
- Free heap memory
- SD card space
- Battery voltage
- LoRa signal strength

## Error Handling
Error codes and their meanings:
- ERR_CAMERA_INIT (1): Camera initialization failed
- ERR_SD_CARD (2): SD card error
- ERR_LORA_INIT (3): LoRa initialization failed
- ERR_RADAR_TIMEOUT (4): Radar communication timeout
- ERR_IMAGE_CAPTURE (5): Image capture failed
- ERR_TRANSMISSION (6): LoRa transmission failed
