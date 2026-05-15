echo "1. Adding uart devices"
./espnowcontrol/build/espnowcontrol add_uart_device /dev/ttyACM0
./espnowcontrol/build/espnowcontrol add_uart_device /dev/ttyACM1

echo "2. Uping interfaces"
sudo ip link set espnow0 up
sudo ip link set espnow1 up
