# EspNowOnLinux

## Project Goal
The primary goal of this project is to make it possible to use the ESP-NOW communication protocol via a Linux host. It bridges the gap between a Linux machine and the ESP-NOW wireless interface using an ESP32 device acting as a USB-CDC adapter.

## Architecture
The system consists of two main components communicating over a USB CDC connection and using shared message formats:
- **Host Application (Linux)**: Translates local Linux networking operations (via a TAP network device) into serial commands.
- **ESP32 Firmware**: Acts as a bridge, translating incoming serial commands from the Linux host into ESP-NOW packets over the air, and vice versa.

### Packet Flow
```text
+-----------------------+              +--------------------+             +-------------------+
|      Linux Host       |              |   ESP32 Adapter    |             |   Remote ESP32    |
|                       |              |                    |             |                   |
|  +-----------------+  |   USB CDC    |  +--------------+  |   ESP-NOW   |  +-------------+  |
|  |   TAP Device    |  |   (Serial)   |  |   Firmware   |  |  (Wireless) |  |   Firmware  |  |
|  |                 |<==================>|              |<=================>|             |  |
|  | (Network Stack) |  |              |  | (usb_cdc_main)| |             |  |             |  |
|  +-----------------+  |              |  +--------------+  |             |  +-------------+  |
+-----------------------+              +--------------------+             +-------------------+
```

## Project Structure
- `host_app/`: The Linux host C++ application. It handles the TAP network device interface, serial port communication with the attached ESP device, and read/write operations.
- `main/`: The ESP-IDF firmware code (e.g., `usb_cdc_main.cpp`, `hello_world_main.c`) targeting ESP32-C3. This firmware configures the Wi-Fi interface, handles ESP-NOW receive/transmit callbacks, and bridges traffic through the USB-CDC interface.
- `messages/`: Shared C++ headers containing data structures (e.g., `packet_to_send`), identifiers, and serialization/deserialization logic used by both the firmware and the host app.
- `test_tools/`: Unit tests (e.g., using Google Test) verifying shared components like message serialization logic.
- `build.sh` / `CMakeLists.txt`: Build and flash scripts supporting different ESP-IDF application targets in the repository.

## Getting Started
The ESP32 firmware can be built and flashed using the provided `build.sh` script (wrapper around ESP-IDF commands). The `host_app` is built separately using CMake on the Linux host. Ensure you have the ESP-IDF environment set up before compiling the device firmware.
