#include <iostream>
#include <string>
#include <fcntl.h>   // Contains file controls like O_RDWR
#include <cerrno>    // Error integer and strerror() function
#include <termios.h> // Contains POSIX terminal control definitions
#include <unistd.h>  // write(), read(), close()
#include <cstring>
#include "messages/message_id.hpp"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"

int main(int argc, char** argv) {
    std::string port = "/dev/ttyACM0";
    if (argc > 1) {
        port = argv[1];
    }

    std::cout << "Opening serial port: " << port << std::endl;

    // Open the serial port. Change device path as needed
    int serial_port = open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);

    if (serial_port < 0) {
        std::cerr << "Error " << errno << " from open: " << strerror(errno) << std::endl;
        return 1;
    }

    // Create new termios struct, we call it 'tty' for convention
    struct termios tty;

    // Read in existing settings, and handle any error
    if(tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcgetattr: " << strerror(errno) << std::endl;
        return 1;
    }

    tty.c_cflag &= ~PARENB; // Clear parity bit, disabling parity (most common)
    tty.c_cflag &= ~CSTOPB; // Clear stop field, only one stop bit used in communication (most common)
    tty.c_cflag &= ~CSIZE; // Clear all bits that set the data size
    tty.c_cflag |= CS8; // 8 bits per byte (most common)
    tty.c_cflag &= ~CRTSCTS; // Disable RTS/CTS hardware flow control (most common)
    tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO; // Disable echo
    tty.c_lflag &= ~ECHOE; // Disable erasure
    tty.c_lflag &= ~ECHONL; // Disable new-line echo
    tty.c_lflag &= ~ISIG; // Disable interpretation of INTR, QUIT and SUSP
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
    tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL); // Disable any special handling of received bytes

    tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
    tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

    // Wait for up to 1s (10 deciseconds), returning as soon as any data is received.
    tty.c_cc[VTIME] = 10;
    tty.c_cc[VMIN] = 0;

    // Set in/out baud rate to be 115200 (For CDC, this doesn't strictly matter for the device, but is standard)
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    // Save tty settings, also checking for error
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cerr << "Error " << errno << " from tcsetattr: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Listening... (Press Ctrl+C to stop)" << std::endl;

    // Allocate memory for read buffer, set size according to your needs
    char read_buf[256];

    while (true) {
        // Read bytes. The behaviour of read() (e.g. does it block?,
        // how long does it block for?) depends on the configuration
        // settings above, specifically VMIN and VTIME
        int num_bytes = read(serial_port, &read_buf, sizeof(read_buf));

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
                        write(serial_port, &message_to_send, sizeof(message_to_send));
                    }
                }
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
                std::cout << std::dec << std::flush;

            }
        }
    }

    close(serial_port);
    return 0; // success
}
