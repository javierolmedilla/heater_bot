#include "esp_http_client.h"
#include "config.h"
#include "config_secret.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include "wifi.h"
#include "driver/gpio.h"


// Buffer para respuesta HTTP
static char http_buffer[MAX_HTTP_OUTPUT_BUFFER];
static int buffer_len = 0;
static int last_update_id = 0;
char mensaje_error[50];

bool encendido_relay = false;
static const char *TAG = "telegram";

// Certificado raíz para api.telegram.org (DigiCert Global Root CA)
extern const uint8_t telegram_cert_pem_start[] asm("_binary_telegram_cert_pem_start");
extern const uint8_t telegram_cert_pem_end[]   asm("_binary_telegram_cert_pem_end");

const char **indices_usuario_telegram = CHAT_IDS();

// Evento handler para HTTP
esp_err_t http_event_handler(esp_http_client_event_t *evt) {
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                if (buffer_len + evt->data_len < MAX_HTTP_OUTPUT_BUFFER) {
                    memcpy(http_buffer + buffer_len, evt->data, evt->data_len);
                    buffer_len += evt->data_len;
                    http_buffer[buffer_len] = 0;
                }
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            break;
    }
    return ESP_OK;
}


void telegram_send_message(const char* bot_token, const char* chat_id, const char* message)
{
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi no conectado");
        return;
    }

    char url[256];
    snprintf(url, sizeof(url),
             "https://api.telegram.org/bot%s/sendMessage",
             bot_token);

    // cuerpo del POST
    char post_data[512];
    snprintf(post_data, sizeof(post_data),
             "chat_id=%s&text=%s",chat_id, message);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 10000,
        .cert_pem = (char *)telegram_cert_pem_start,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Mensaje enviado correctamente");
    } else {
        ESP_LOGE(TAG, "Error enviando mensaje: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

bool usuarioValido(char *chat_id_str) {
    int n = 0;
    while(indices_usuario_telegram[n] != NULL) { 
        if(strcmp(chat_id_str,indices_usuario_telegram[n]) == 0) 
            return true;
        n++;
    }
    return false;
}

// Procesar mensajes recibidos
void process_telegram_message(cJSON *message) {
    cJSON *chat = cJSON_GetObjectItem(message, "chat");
    cJSON *text = cJSON_GetObjectItem(message, "text");
    
    if (!chat || !text) {
        return;
    }
    
    cJSON *chat_id = cJSON_GetObjectItem(chat, "id");
    cJSON *chat_name = cJSON_GetObjectItem(chat, "first_name");
    
    if (chat_id && text) {
        char chat_id_str[32];
        char chat_name_str[32];
        snprintf(chat_id_str, sizeof(chat_id_str), "%.0f", chat_id->valuedouble);
        snprintf(chat_name_str, sizeof(chat_name_str), "%s", chat_name->valuestring);

      //  if (strcmp(chat_id_str, CHAT_ID) == 0)
           if (usuarioValido(chat_id_str))
           ESP_LOGI(TAG, "Mensaje recibido de %s, %s: %s", chat_id_str, chat_name_str, text->valuestring);
        else {
           char array1[] = "Usuario desconocido: ";
           strcpy(mensaje_error, array1);      
           strcat(mensaje_error, chat_name_str);
           strcat(mensaje_error, chat_id_str);
           ESP_LOGI(TAG, "Usuario desconocido");
           const char* mensaje = mensaje_error;
           telegram_send_message(BOT_TOKEN, CHAT_ID, mensaje);
           return;
        }
        
        // Procesar comandos
        if (strcmp(text->valuestring, "/start") == 0) {
            telegram_send_message(BOT_TOKEN, chat_id_str, "ENVIADO /START");
            ESP_LOGI(TAG, "Recibido un /start");
        } 
        else if (strcmp(text->valuestring, "/help") == 0) {
            ESP_LOGI(TAG, "Recibido un /help");
            telegram_send_message(BOT_TOKEN, chat_id_str,
                "Comandos disponibles:\n"
                "/help - Ayuda\n"
                "/status - Estado de la calefacción\n"
                "/encender - Encender Calefacción\n"
                "/apagar - Apagar Calefacción\n");
        }
        else if (strcmp(text->valuestring, "/status") == 0) {
            char status_msg[128];
            snprintf(status_msg, sizeof(status_msg), 
                     "ESP32 activo\nMemoria libre: %lu bytes",
                     esp_get_free_heap_size());
            int estado = gpio_get_level(RELAY_GPIO);
            if (encendido_relay) {
                telegram_send_message(BOT_TOKEN, chat_id_str, "Status: Calefacción encendida");
            } else {
                telegram_send_message(BOT_TOKEN, chat_id_str, "Status: Calefacción apagada");
            }
            ESP_LOGI(TAG, "/status %i", estado);
        }
        else if (strcmp(text->valuestring, "/info") == 0) {
            ESP_LOGI(TAG, "Recibido un /info");
        }
        else if (strcmp(text->valuestring, "/encender") == 0) {
            // Aquí puedes controlar un GPIO
            gpio_set_level(RELAY_GPIO, 0);
            ESP_LOGI(TAG, "Recibido un /encender %i",GPIO_NUM_14);
            encendido_relay = true;
            telegram_send_message(BOT_TOKEN, chat_id_str, "Calefacción encendida");
            telegram_send_message(BOT_TOKEN, CHAT_ID, "Calefacción encendida");
        }
        else if (strcmp(text->valuestring, "/apagar") == 0) {
            gpio_set_level(RELAY_GPIO, 1);
            ESP_LOGI(TAG, "Recibido un /apagar %i", GPIO_NUM_14);
            encendido_relay = false;
            telegram_send_message(BOT_TOKEN, chat_id_str, "Calefacción apagada");
            telegram_send_message(BOT_TOKEN, CHAT_ID, "Calefacción apagada");
        }
        else {
            char response[256];
            snprintf(response, sizeof(response), "Recibido: %s", text->valuestring);
            ESP_LOGI(TAG, "Recibido un %s",text->valuestring);
        }
    }
}

// Obtener actualizaciones de Telegram
void get_telegram_updates() {
    if (!wifi_connected) {
        ESP_LOGW(TAG, "WiFi no conectado");
        return;
    }

    char url[512];
    snprintf(url, sizeof(url), "%s%s/getUpdates?offset=%d&timeout=%d", 
             TELEGRAM_API, BOT_TOKEN, last_update_id + 1, TELEGRAM_POLLING_TIMEOUT);

    buffer_len = 0;
    memset(http_buffer, 0, MAX_HTTP_OUTPUT_BUFFER);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event_handler,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .cert_pem = (char *)telegram_cert_pem_start,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Error al inicializar cliente HTTP");
        return;
    }
    
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP Status = %d, content_length = %lld",
                status_code,
                esp_http_client_get_content_length(client));
        
        if (status_code == 200 && buffer_len > 0) {
            cJSON *json = cJSON_Parse(http_buffer);
            if (json) {
                cJSON *ok = cJSON_GetObjectItem(json, "ok");
                if (ok && cJSON_IsTrue(ok)) {
                    cJSON *result = cJSON_GetObjectItem(json, "result");
                    if (result && cJSON_IsArray(result)) {
                        int array_size = cJSON_GetArraySize(result);
             //           ESP_LOGI(TAG, "Recibidas %d actualizaciones", array_size);
                        
                        for (int i = 0; i < array_size; i++) {
                            cJSON *update = cJSON_GetArrayItem(result, i);
                            cJSON *update_id = cJSON_GetObjectItem(update, "update_id");
                            cJSON *message = cJSON_GetObjectItem(update, "message");
                            
                            if (update_id) {
                                if (cJSON_IsNumber(update_id)) {
                                    int uid = (int)update_id->valuedouble;
                                    if (uid > last_update_id) {
                                        last_update_id = uid;
                                    }
                                }
                            }
                            
                            if (message) {
                                process_telegram_message(message);
                            }
                        }
                    }
                }
                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Error parseando JSON");
            }
        }
    } else {
        ESP_LOGE(TAG, "Error en HTTP request: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
}

// Tarea principal del bot
void telegram_bot_task(void *pvParameters) {
    ESP_LOGI(TAG, "Esperando conexión WiFi...");

    // Esperar a que WiFi esté conectado
    while (!wifi_connected) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    ESP_LOGI(TAG, "Bot de Telegram iniciado y listo!");

    while (1) {
        if (wifi_connected) {
            get_telegram_updates();
        }
        vTaskDelay(pdMS_TO_TICKS(TELEGRAM_UPDATE_INTERVAL));
    }
}
