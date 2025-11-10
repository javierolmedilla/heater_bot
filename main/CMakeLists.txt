idf_component_register(SRCS "main.c" "telegram.c" "wifi.c" 
                      INCLUDE_DIRS "."
                      EMBED_TXTFILES "certs/telegram_cert.pem"
                      REQUIRES json esp_driver_gpio driver nvs_flash esp_wifi esp_http_client json mbedtls)

