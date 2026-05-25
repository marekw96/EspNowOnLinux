#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "config.h"
#include "messages/start_device.hpp"
#include "messages/start_host.hpp"
#include "messages/received_packet.hpp"
#include "messages/packet_to_send.hpp"
#include "messages/ping.hpp"
#include "utility/receiving_buffer.hpp"
#include <algorithm>


SemaphoreHandle_t jtag_tx_semaphore;

extern "C" {
#include <stdarg.h>

static void push_to_uart_with_synchro(void* data, size_t size){
    if(xSemaphoreTake(jtag_tx_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE){
        usb_serial_jtag_write_bytes(data, size, pdMS_TO_TICKS(100));
        xSemaphoreGive(jtag_tx_semaphore);
    }
}

static void write_log_to_uart(const char* message, size_t size){
    auto size_network = host_to_network(static_cast<uint32_t>(size));
    if(xSemaphoreTake(jtag_tx_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE){
        usb_serial_jtag_write_bytes("i", 1, pdMS_TO_TICKS(100));
        usb_serial_jtag_write_bytes(&size_network, sizeof(size_network), pdMS_TO_TICKS(100));
        usb_serial_jtag_write_bytes(message, size, pdMS_TO_TICKS(100));
        xSemaphoreGive(jtag_tx_semaphore);
    }
}

static void log_to_uart(const char* format, ...){
    va_list args;
    va_start(args, format);
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    write_log_to_uart(buffer, sizeof(buffer));
    va_end(args);
}
}


static QueueHandle_t receive_queue;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const char *TAG = "USB_CDC";
#define ESPNOW_MAXDELAY 512

struct __attribute__((packed)) packet{
    uint8_t id;
    uint8_t src_mac[6];
    uint32_t payload_size;
    uint8_t payload[250];
}__attribute__((packed));

struct occupied_packet {
    bool is_free;
    packet data;
};

static occupied_packet packets[15];

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    example_espnow_event_t evt;
    example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    if (IS_BROADCAST_ADDR(des_addr)) {
        /* If added a peer with encryption before, the receive packets may be
         * encrypted as peer-to-peer message or unencrypted over the broadcast channel.
         * Users can check the destination address to distinguish it.
         */
        ESP_LOGD(TAG, "Receive broadcast ESPNOW data");
    } else {
        ESP_LOGD(TAG, "Receive unicast ESPNOW data");
    }

    auto it = std::find_if(std::begin(packets), std::end(packets), [](const occupied_packet& p){
        return p.is_free;
    });
    if(it != std::end(packets)){
        it->is_free = false;
        memcpy(it->data.src_mac, mac_addr, 6);
        it->data.payload_size = len;
        memcpy(it->data.payload, data, len);
        uint8_t idx = std::distance(std::begin(packets), it);
        if (xQueueSend(receive_queue, &idx, ESPNOW_MAXDELAY) != pdTRUE) {
            ESP_LOGW(TAG, "Send receive queue fail");
        }
        // log_to_uart("Received packet with size %d", len);
    } else {
        log_to_uart("No space to store received packet");
    }
}

static esp_err_t example_espnow_init(void)
{
    receive_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(uint8_t));
    if (receive_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif

#ifdef CONFIG_ESPNOW_PMK
    if (strlen(CONFIG_ESPNOW_PMK) > 0) {
        ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
    }
#endif

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = reinterpret_cast<esp_now_peer_info_t *>(malloc(sizeof(esp_now_peer_info_t)));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vQueueDelete(receive_queue);
        receive_queue = NULL;
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    return ESP_OK;
}

int32_t handle_packet_from_jtag(std::span<uint8_t> buffer){
    message_id id = static_cast<message_id>(buffer[0]);
    if(id == message_id::PACKET_TO_SEND) {
        if(buffer.size() < 11){
            return -1;
        }

        auto payload_size = network_to_host(*reinterpret_cast<const uint32_t*>(buffer.data() + 7));
        // ESP_LOGI(TAG, "Received packet with size %d, payload", payload_size);
        if(payload_size + 11 > buffer.size()){
            // log_to_uart("buffer[%d] is smaller than expected packet size[%d]", buffer.size(), payload_size + 11);
            return -1;
        }

        packet_to_send packet = io<packet_to_send>::deserialize(buffer);

        if(esp_now_send(packet.destination_mac, packet.data.data(), packet.data.size()) != ESP_OK) {
            log_to_uart("Failed to send packet");
        }

        return 11 + packet.data.size();
    }

    log_to_uart("Unknown message id: %d", id);
    return 0;
}

void jtag_receive_task(void *pvParameters) {
    receiving_buffer<512> buffer;

    while (1) {
        auto free_space = buffer.get_writable();
        int bytes_read = usb_serial_jtag_read_bytes(free_space.data(), free_space.size(), pdMS_TO_TICKS(100));
        buffer.commit(bytes_read);

        auto data_span = buffer.get_data();

        while(!data_span.empty()) {
            auto processed_bytes = handle_packet_from_jtag(data_span);
            if(processed_bytes == -1 ){
                // log_to_uart("Wait for remainig data");
                buffer.move_data_to_front();
                break;
            }

            if(processed_bytes == 0) {
                log_to_uart("Failed to handle packet");
                buffer.reset();
                break;
            }

            buffer.consume(processed_bytes);
            data_span = buffer.get_data();
        }
    }
}

void ping_task(void *pvParameters){
    while(1){
        ping p;
        push_to_uart_with_synchro(&p, sizeof(p));
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void usb_main(void)
{
    // 0. Clear packets
    for(auto& packet : packets) {
        packet.is_free = true;
    }

    // 1. Configure the USB Serial/JTAG driver
    usb_serial_jtag_driver_config_t usb_config = {
        .tx_buffer_size = 256,
        .rx_buffer_size = 256,
    };

    // 2. Install the driver
    jtag_tx_semaphore = xSemaphoreCreateMutex();
    esp_err_t err = usb_serial_jtag_driver_install(&usb_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Serial JTAG driver: %s", esp_err_to_name(err));
        return;
    }

    start_device message = {
        .id = message_id::START_DEVICE,
        .type = device_type::ESP32_C3
    };

    uint8_t buffer[256];

    while (1) {
        push_to_uart_with_synchro(&message, sizeof(message));

        int bytes_read = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), pdMS_TO_TICKS(100));
        if (bytes_read > 0) {
            message_id id = static_cast<message_id>(buffer[0]);
            if(id == message_id::START_HOST) {
                break;
            }
        }
    }

    ESP_LOGI(TAG, "Handshake complete");

    example_wifi_init();
    example_espnow_init();

    xTaskCreate(jtag_receive_task, "jtag_receive_task", 2048, NULL, 5, NULL);
    xTaskCreate(ping_task, "ping_task", 512, NULL, 5, NULL);

    ESP_LOGI(TAG, "HW init done.");

    uint8_t index;
    while (1) {
        if (xQueueReceive(receive_queue, &index, portMAX_DELAY) == pdTRUE) {
            auto& r_packet = packets[index];

            auto total_size = 1 + 6 + 4 + r_packet.data.payload_size;
            r_packet.data.id = static_cast<uint8_t>(message_id::RECEIVED_PACKET);
            r_packet.data.payload_size = host_to_network(r_packet.data.payload_size);

            push_to_uart_with_synchro(&r_packet.data, total_size);
            r_packet.is_free = true;
        }
    }
}

extern "C" void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    usb_main();
}