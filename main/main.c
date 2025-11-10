#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"    
#include "config.h"
#include "config_secret.h"
#include "telegram.h"
#include "wifi.h"
#include <time.h>

static const char *TAG = "main";

#define BLINK_GPIO 2
#define LED_GPIO       2       // LED integrado


// Función para obtener la hora y guardarla en un string
const char* obtenerHora(char* buffer, int buffer_size) {
    // Obtener el tiempo actual
    time_t segundos = time(NULL);
    struct tm* tiempo_local = localtime(&segundos);

    // Formatear la hora en un string
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tiempo_local);
    return buffer;
}



void app_main(void) {
    ESP_LOGI(TAG, "Arrancando aplicación");
    wifi_connected = false;
    start_wifi();
    //xTaskCreate(blink_task, "blink_task", 1024*2, NULL, 5, NULL);

    // Espera unos segundos para conectarse a Wi-Fi
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    print_dns_info();
    // Envía mensaje a Telegram
    telegram_send_message(BOT_TOKEN, CHAT_ID, "ESP32 conectado y funcionando!");
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(RELAY_GPIO, 1);
    // Crear tarea del bot
    xTaskCreate(&telegram_bot_task, "telegram_bot_task", 8192, NULL, 5, NULL);
}

