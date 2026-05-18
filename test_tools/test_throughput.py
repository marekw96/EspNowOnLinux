import socket
import threading
import time
import sys

IFACE_0 = "espnow0"
IFACE_1 = "espnow1"

MAC_0 = b'\x11\x22\x33\x44\x55\x66'
MAC_BROADCAST = b'\xff\xff\xff\xff\xff\xff'
ETH_TYPE = b'\x88\xb5'
PAYLOAD_SIZE = 20
NUM_PACKETS = 100#256

def send_packets():
    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
        sock.bind((IFACE_0, 0))
    except Exception as e:
        print(f"Send bind error: {e}")
        return

    print(f"Starting to send {NUM_PACKETS} packets from {IFACE_0}...")

    for i in range(NUM_PACKETS):
        # Payload: first byte is index (0-255), followed by zero padding
        payload = bytes([i]) + b'\x00' * (PAYLOAD_SIZE - 1)
        frame = MAC_BROADCAST + MAC_0 + ETH_TYPE + payload
        sock.send(frame)
        # Small sleep can be added if packets are dropped due to buffer overflow
        time.sleep(0.01)

    sock.close()
    print("All packets sent.")

def receive_packets():
    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x88b5))
        sock.bind((IFACE_1, 0))
    except Exception as e:
        print(f"Receive bind error: {e}")
        return 0, 0.0, set(range(NUM_PACKETS))

    sock.settimeout(3.0)
    received_indices = set()

    start_time = None
    end_time = None

    print(f"Listening on {IFACE_1}...")

    while len(received_indices) < NUM_PACKETS:
        try:
            data, _ = sock.recvfrom(2048)
            if len(data) >= 14 + PAYLOAD_SIZE:
                dest_mac = data[0:6]
                src_mac = data[6:12]
                eth_type = data[12:14]

                if dest_mac == MAC_BROADCAST and eth_type == ETH_TYPE:
                    if start_time is None:
                        start_time = time.time()

                    payload = data[14:14+PAYLOAD_SIZE]
                    idx = payload[0]
                    received_indices.add(idx)
                    end_time = time.time()

        except socket.timeout:
            break

    sock.close()

    if start_time and end_time and len(received_indices) > 1:
        duration = end_time - start_time
    else:
        duration = 0.0

    missing = set(range(NUM_PACKETS)) - received_indices
    return len(received_indices), duration, missing

def run_test():
    class Receiver(threading.Thread):
        def __init__(self):
            super().__init__()
            self.count = 0
            self.duration = 0.0
            self.missing = set()

        def run(self):
            self.count, self.duration, self.missing = receive_packets()

    receiver = Receiver()
    receiver.start()

    # Wait for listener to bind and be ready
    time.sleep(1.0)

    sender_thread = threading.Thread(target=send_packets)
    sender_thread.start()

    sender_thread.join()
    receiver.join()

    print("-" * 40)
    print("Test Results:")
    print(f"Received {receiver.count} / {NUM_PACKETS} packets.")
    if receiver.duration > 0:
        print(f"Time taken to receive: {receiver.duration:.4f} seconds ({receiver.duration * 1000:.2f} ms).")
        print(f"Throughput: {receiver.count / receiver.duration:.2f} packets/second.")
    if receiver.missing:
        print(f"Missing indices: {sorted(list(receiver.missing))}")

    if receiver.count == NUM_PACKETS:
        print("SUCCESS")
        return True
    else:
        print("FAILED")
        return False

if __name__ == "__main__":
    success = run_test()
    sys.exit(0 if success else 1)
