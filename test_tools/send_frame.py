import socket

# Interface to send from (change if needed, e.g., "eth0", "wlan0")
INTERFACE = "espnow0"

# MAC addresses
dest_mac  = b'\xff\xff\xff\xff\xff\xff'
src_mac = b'\x11\x22\x33\x44\x55\x66'

# EtherType (0x0800 = IPv4, but here it's just a placeholder)
eth_type = b'\x88\xb5'

# Payload
payload = b"message 1234567890"

# Build Ethernet frame
frame = dest_mac + src_mac + eth_type + payload

# Create raw socket
sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)

# Bind to interface
sock.bind((INTERFACE, 0))

# Send frame
sock.send(frame)

print("Ethernet frame sent")
