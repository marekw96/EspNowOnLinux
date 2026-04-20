# EspNowOnLinux Features Checklist

This document tracks the features supported by the EspNowOnLinux project. Check off items as they are implemented.

## 1. ESP-NOW Core Features
- [ ] Initialize ESP-NOW protocol completely
- [ ] Transmit and receive ESP-NOW data packets
- [ ] Broadcast message support (e.g., for device discovery and announcements)
- [ ] Unicast message support (direct peer-to-peer data transfer)
- [ ] Hardware-level delivery callbacks (Success/Fail transmission status propagation)
- [ ] Secure Encryption: Encrypted ESP-NOW communication (LMK/PMK support)
- [ ] Configurable Wi-Fi channel and interface

## 2. Multi-Device & Dynamic Discovery
- [ ] Auto-discovery of surrounding ESP-NOW nodes (zero-config networking)
- [ ] Dynamic peer registration (cache and automatically add new peers upon detection)
- [ ] Support communicating with multiple independent ESP-NOW nodes simultaneously
- [ ] Connection health/presence monitoring (timeout and drop unresponsive peers)
- [ ] Ping/RSSI measurement (track connection strength to each node)

## 3. Host System & Usability (Linux side)
- [ ] Transparent Network Device: Emulate TAP interface (ESP-NOW nodes act as standard IP/Ethernet devices)
- [ ] Plug-and-Play: Auto-detect ESP32 serial/USB connection on start
- [ ] Auto-recovery: Reconnect seamlessly if ESP32 device resets or is unplugged
- [ ] Easy-to-use CLI: Command to list currently discovered ESP-NOW peers and their signal strengths
- [ ] Host-driven configuration: Set Wi-Fi channel and encryption keys from the Linux CLI
- [ ] Clear Diagnostics: Human-readable logging for peer joins, drops, and packet drops

## 4. Physical Transport Layer & Serialization
- [ ] High-speed USB-CDC / Serial communication
- [ ] Reliable binary serialization / framing (C++ structs to binary)
- [ ] Separation of Control messages (e.g., "Add Peer") and Data messages (network packets)
