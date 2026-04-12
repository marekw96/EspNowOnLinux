#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"

static const char *TAG = "USB_CDC";

void app_main(void)
{
    // 1. Configure the USB Serial/JTAG driver
    usb_serial_jtag_driver_config_t usb_config = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256,
    };

    // 2. Install the driver
    esp_err_t err = usb_serial_jtag_driver_install(&usb_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Serial JTAG driver: %s", esp_err_to_name(err));
        return;
    }

    const char *message = "Hello from ESP32-C3 over USB CDC mode!\r\n";

    while (1) {
        // 3. Write data to the host
        // The last parameter is the timeout in ticks in case the TX buffer is full
        int bytes_written = usb_serial_jtag_write_bytes(message, strlen(message), pdMS_TO_TICKS(100));
        ESP_LOGI(TAG, "Bytes written: %d", bytes_written);

        if (bytes_written < 0) {
            ESP_LOGE(TAG, "Failed to write data");
        }

        // Wait 1 second before sending the next message
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
