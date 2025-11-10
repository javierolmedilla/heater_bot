#ifndef TELEGRAM_H
#define TELEGRAM_H

void get_telegram_updates();
void telegram_send_message(const char* bot_token, const char* chat_id, const char* message);
void telegram_bot_task(void *pvParameters);
extern bool encendido_relay;

#endif
