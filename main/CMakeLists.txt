idf_component_register(SRCS "usb_cdc_main.cpp"
                    PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_driver_usb_serial_jtag
                    INCLUDE_DIRS "." "../")