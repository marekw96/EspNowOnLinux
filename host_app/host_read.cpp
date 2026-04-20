#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>
#include "serial_port.hpp"
#include "messages/message_id.hpp"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"
#include "messages/received_packet.hpp"

int main(int argc, char** argv) {
    std::string port = "/dev/ttyACM0";
    if (argc > 1) {
        port = argv[1];
    }

    std::cout << "Opening serial port: " << port << std::endl;

    try {
        serial_port sp(port);

        std::cout << "Listening... (Press Ctrl+C to stop)" << std::endl;

        // Allocate memory for read buffer, set size according to your needs
        unsigned char read_buf[256];

        while (true) {
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
                    std::cout << "[Received] ";
                    for(auto b: message.data) {
                        std::cout << b;
                    }
                    std::cout << std::endl;
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
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0; // success
}
