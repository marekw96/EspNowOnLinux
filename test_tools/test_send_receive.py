import socket
import threading
import time
import sys

IFACE_0 = "espnow0"
IFACE_1 = "espnow1"

MAC_0 = b'\x11\x22\x33\x44\x55\x66'
MAC_1 = b'\xaa\xbb\xcc\xdd\xee\xff'
MAC_BROADCAST = b'\xff\xff\xff\xff\xff\xff'
ETH_TYPE = b'\x88\xb5'

# Build expected frames
# Frame from espnow0 to espnow1 (dest = MAC_BROADCAST, src = MAC_0)
payload_0_to_1 = b"Hello from espnow0 to espnow1!"
frame_0_to_1 = MAC_BROADCAST + MAC_0 + ETH_TYPE + payload_0_to_1

# Frame from espnow1 to espnow0 (dest = MAC_BROADCAST, src = MAC_1)
payload_1_to_0 = b"Hello back from espnow1 to espnow0!"
frame_1_to_0 = MAC_BROADCAST + MAC_1 + ETH_TYPE + payload_1_to_0


def send_frame(interface: str, frame: bytes):
    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
        sock.bind((interface, 0))
        sock.send(frame)
    except PermissionError:
        print("Permission denied. Run as root.")
    except OSError as e:
        print(f"Failed to bind/send on {interface}: {e}")
    finally:
        if 'sock' in locals():
            sock.close()


def wait_for_frame(interface: str, expected_payload: bytes, timeout: float = 2.0) -> tuple:
    try:
        # ETH_P_ALL is 0x0003
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(0x88b5))
        sock.bind((interface, 0))
    except PermissionError:
        return False, "Permission denied. Run as root."
    except OSError as e:
        return False, f"Failed to bind on {interface}: {e}"

    end_time = time.time() + timeout
    mismatched_packets = []

    try:
        while time.time() < end_time:
            remaining = end_time - time.time()
            if remaining <= 0:
                break
            sock.settimeout(remaining)

            try:
                data, _ = sock.recvfrom(2048)
                print("Received frame:", data)
                # Check for an exact match of the payload (starts at byte 14)
                if len(data) >= 14 + len(expected_payload) and data[14:14+len(expected_payload)] == expected_payload:
                    return True, "Success"
                else:
                    mismatched_packets.append(data)
            except socket.timeout:
                break
    finally:
        sock.close()

    if not mismatched_packets:
        return False, "Timeout. No packets received on this interface."
    else:
        last_mismatch = mismatched_packets[-1]
        msg = f"Received {len(mismatched_packets)} other packet(s) but no match found.\n"
        msg += f"        Expected payload length: {len(expected_payload)}, Last received length: {len(last_mismatch)}\n"
        msg += f"        Expected payload (hex)     : {expected_payload.hex()}\n"
        msg += f"        Expected payload (str)     : {expected_payload.decode()}\n"
        msg += f"        Last received payload (hex): '{last_mismatch[14:14+len(expected_payload)].hex() if len(last_mismatch) >= 14 else last_mismatch.hex()}'\n"
        msg += f"        Last received payload (str): '{last_mismatch[14:14+len(expected_payload)].decode() if len(last_mismatch) >= 14 else last_mismatch.decode()}'\n"
        return False, msg


def test_communication():
    print(f"Testing {IFACE_0} -> {IFACE_1}...")

    # 1. Listen on IFACE_1
    result_0_to_1 = [False, ""]
    def listen_1():
        result_0_to_1[0], result_0_to_1[1] = wait_for_frame(IFACE_1, payload_0_to_1)

    t1 = threading.Thread(target=listen_1)
    t1.start()

    time.sleep(0.5) # Give listener time to start

    # 2. Send from IFACE_0
    send_frame(IFACE_0, frame_0_to_1)
    t1.join()

    if result_0_to_1[0]:
        print(f"SUCCESS: {IFACE_0} -> {IFACE_1}")
    else:
        print(f"FAILED: {IFACE_0} -> {IFACE_1}")
        print(f"Reason: {result_0_to_1[1]}")

    print(f"\nTesting {IFACE_1} -> {IFACE_0}...")

    # 3. Listen on IFACE_0
    result_1_to_0 = [False, ""]
    def listen_0():
        result_1_to_0[0], result_1_to_0[1] = wait_for_frame(IFACE_0, payload_1_to_0)

    t0 = threading.Thread(target=listen_0)
    t0.start()

    time.sleep(0.5) # Give listener time to start

    # 4. Send from IFACE_1
    send_frame(IFACE_1, frame_1_to_0)
    t0.join()

    if result_1_to_0[0]:
        print(f"SUCCESS: {IFACE_1} -> {IFACE_0}")
    else:
        print(f"FAILED: {IFACE_1} -> {IFACE_0}")
        print(f"Reason: {result_1_to_0[1]}")

    return result_0_to_1[0] and result_1_to_0[0]


if __name__ == "__main__":
    success = test_communication()
    print()
    if success:
        print("All tests passed!")
        sys.exit(0)
    else:
        print("Some tests failed!")
        sys.exit(1)
