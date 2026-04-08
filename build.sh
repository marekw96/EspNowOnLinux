# Set the target to ESP32-C3
idf.py set-target esp32c3

# Build the project
idf.py build

# Flash the project to your device (replace /dev/ttyUSB0 with your actual port) and open the monitor
idf.py -p /dev/ttyUSB0 flash monitor
