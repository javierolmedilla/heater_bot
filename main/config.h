#ifndef CONFIG_H
#define CONFIG_H


// ========================================
// Configuración GPIO (opcional)
// ========================================
#define LED_PIN 2
#define RELAY_GPIO     14
#define LED_GPIO       2
#define BLINK_GPIO 2

// ========================================
// Configuración de red
// ========================================
#define TELEGRAM_API "https://api.telegram.org/bot"
#define TELEGRAM_POLLING_TIMEOUT 30
#define TELEGRAM_UPDATE_INTERVAL 1000

// ========================================
// Configuración del sistema
// ========================================
#define MAX_HTTP_OUTPUT_BUFFER 4096
#define HTTP_TIMEOUT_MS 35000

#endif // CONFIG_H
