#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    printf("Hello world for ESP32-C3!\n");

    int count = 0;
    while (1) {
        printf("Running... count: %d\n", count++);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
