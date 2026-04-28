#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include <iomanip>
#include <poll.h>
#include "serial_port.hpp"
#include "tap_device.hpp"

#include "messages/utility.hpp"
#include "messages/message_id.hpp"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"
#include "messages/received_packet.hpp"

struct eth_header {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
};

uint8_t tap_buffer[500] = {0};

int main(int argc, char** argv) {
    std::string port = "/dev/ttyACM0";
    if (argc > 1) {
        port = argv[1];
    }

    std::cout << "Opening serial port: " << port << std::endl;

    try {
        serial_port sp(port);
        tap_device tap("espnow0");

        struct pollfd fds[2];
        fds[0].fd = sp.get_fd();
        fds[0].events = POLLIN;
        fds[1].fd = tap.get_fd();
        fds[1].events = POLLIN;

        std::cout << "Listening... (Press Ctrl+C to stop)" << std::endl;

        // Allocate memory for read buffer, set size according to your needs
        unsigned char read_buf[256];

        while (true) {

            int ready = poll(fds, 2, -1);

            if(ready < 0) {
                std::cerr << "Error reading: " << strerror(errno) << std::endl;
            }

            if(fds[0].revents & POLLIN) {
                // Read bytes. The behaviour of read() (e.g. does it block?,
                // how long does it block for?) depends on the configuration
                // settings above, specifically VMIN and VTIME
                int num_bytes = sp.read(read_buf);

                // n is the number of bytes read. n may be 0 if no bytes were received, and can also be -1 to signal an error.
                if (num_bytes < 0) {
                    std::cerr << "Error reading: " << strerror(errno) << std::endl;
                    break;
                }

                if (num_bytes > 0) {
                    std::cout << "Read " << num_bytes << " bytes: ";
                    message_id id = static_cast<message_id>(read_buf[0]);
                    if(id == message_id::START_DEVICE) {
                        std::cout << "Start device message" << std::endl;
                        if(num_bytes == sizeof(start_device)) {
                            if( memcmp(read_buf + 1, "espnowonlinux", 13) == 0) {
                                start_device* message = reinterpret_cast<start_device*>(read_buf);
                                std::cout << "Device type: " << static_cast<int>(message->type) << std::endl;

                                start_host message_to_send;
                                sp.write({reinterpret_cast<const unsigned char*>(&message_to_send), sizeof(message_to_send)});
                            }
                        }
                    }
                    else if(id == message_id::LOG_INFO) {
                        auto* message = reinterpret_cast<char*>(read_buf);
                        message[num_bytes-1] = '\0';
                        std::cout << "[Info] " << message + 1 << std::endl;
                    }
                    else if (id == message_id::RECEIVED_PACKET) {
                        auto message = io<received_packet>::deserialize(std::span<const unsigned char>(read_buf, num_bytes));
                        std::cout << "[Received] packet from " << message.mac << " with " << message.data.size() << " bytes" << std::endl;

                        eth_header eth;
                        for(auto& d : eth.dest_mac)
                            d = 0xff;
                        memcpy(eth.src_mac, message.mac, 6);
                        eth.ethertype = 0x88B5;

                        memcpy(tap_buffer, &eth, sizeof(eth));
                        memcpy(tap_buffer + sizeof(eth), message.data.data(), message.data.size());
                        tap.write(std::span<const unsigned char>(tap_buffer, sizeof(eth) + message.data.size()));

                    }
                    else {
                        // we read some bytes, let's print them
                        for(int i = 0; i < num_bytes; ++i) {
                            std::cout << std::hex << static_cast<unsigned>(read_buf[i]) << ' ';
                        }
                        std::cout << std::dec << std::flush;

                        for(int i = 0; i < num_bytes; ++i) {
                            std::cout << static_cast<char>(read_buf[i]);
                        }
                        std::cout << std::dec << std::endl << std::flush;

                    }
                }
            }

            if(fds[1].revents & POLLIN) {
                uint8_t eth_buffer[1500] = {0};
                int eth_bytes = tap.read({eth_buffer, sizeof(eth_buffer)});
                if (eth_bytes > 0) {
                    uint16_t ethertype = network_to_host(*reinterpret_cast<uint16_t*>(eth_buffer + 12));
                    if(ethertype == 0x88B5) {
                        std::cout << "Read " << eth_bytes << " bytes from tap" << std::endl;

                        for(int i = 0; i < eth_bytes; ++i) {
                            std::cout << eth_buffer[i] << " " << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(eth_buffer[i]) << " " << (i % 16 == 15 ? "\n" : "");
                        }
                        std::cout << std::dec << std::flush << std::endl;
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0; // success
}
