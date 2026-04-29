#!/bin/bash

APP=${1:-espnow}
DEVICE=${2:-/dev/ttyACM0}

if [ "$APP" = "usb_cdc" ]; then
    export BUILD_APP="usb_cdc"
    BUILD_DIR="build_usb_cdc"
else
    export BUILD_APP="espnow"
    BUILD_DIR="build_espnow"
fi

# Set the target to ESP32-C3
idf.py -B $BUILD_DIR set-target esp32c3

# Build the project
idf.py -B $BUILD_DIR build

# Flash the project to your device (replace /dev/ttyUSB0 with your actual port) and open the monitor
idf.py -B $BUILD_DIR -p $DEVICE flash monitor
