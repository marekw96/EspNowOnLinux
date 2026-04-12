if("$ENV{BUILD_APP}" STREQUAL "usb_cdc")
    idf_component_register(SRCS "usb_cdc_main.c"
                        INCLUDE_DIRS ".")
else()
    idf_component_register(SRCS "hello_world_main.c"
                        PRIV_REQUIRES nvs_flash esp_event esp_netif esp_wifi
                        INCLUDE_DIRS ".")
endif()