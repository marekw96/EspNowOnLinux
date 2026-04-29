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


static QueueHandle_t receive_queue;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const char *TAG = "USB_CDC";
#define ESPNOW_MAXDELAY 512

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

    evt.id = EXAMPLE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = reinterpret_cast<uint8_t*>(malloc(len));
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(receive_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}

static esp_err_t example_espnow_init(void)
{
    receive_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
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

void jtag_receive_task(void *pvParameters) {
    uint8_t buffer[256];
    while (1) {
        int bytes_read = usb_serial_jtag_read_bytes(buffer, sizeof(buffer), pdMS_TO_TICKS(100));
        if (bytes_read > 0) {
            message_id id = static_cast<message_id>(buffer[0]);
            if(id == message_id::PACKET_TO_SEND) {
                packet_to_send packet = io<packet_to_send>::deserialize(std::span<const unsigned char>(buffer, bytes_read));
                ESP_LOGI(TAG, "Received packet to send to mac: %x%x%x%x%x%x %d bytes", packet.destination_mac[0], packet.destination_mac[1], packet.destination_mac[2], packet.destination_mac[3], packet.destination_mac[4], packet.destination_mac[5], packet.data.size());
                if(esp_now_send(packet.destination_mac, packet.data.data(), packet.data.size()) != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send packet");
                }
            }
        }
    }
}

void usb_main(void)
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

    start_device message = {
        .id = message_id::START_DEVICE,
        .type = device_type::ESP32_C3
    };

    uint8_t buffer[256];

    while (1) {
        usb_serial_jtag_write_bytes(&message, sizeof(message), pdMS_TO_TICKS(100));

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

    ESP_LOGI(TAG, "HW init done.");

    example_espnow_event_t evt;
    while (1) {
        if (xQueueReceive(receive_queue, &evt, portMAX_DELAY) == pdTRUE) {
            if (evt.id == EXAMPLE_ESPNOW_RECV_CB) {
                example_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                received_packet packet;
                memcpy(packet.mac, recv_cb->mac_addr, sizeof(packet.mac));
                packet.data.insert(packet.data.end(), recv_cb->data, recv_cb->data + recv_cb->data_len);
                auto buffer = io<received_packet>::serialize(packet);
                usb_serial_jtag_write_bytes(buffer.data(), buffer.size(), pdMS_TO_TICKS(100));

                free(recv_cb->data);
            }
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