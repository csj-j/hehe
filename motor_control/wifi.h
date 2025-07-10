#ifndef WIFI_H
#define WIFI_H

#include <termios.h>

// 配置参数
#define WIFI_SSID "HONOR 80"
#define WIFI_PASSWORD "liyou081909"
#define MQTT_SERVER "mqtt.heclouds.com"
#define MQTT_PORT 1883
#define PRODUCT_ID "1bUie6VQTe"
#define DEVICE_ID "d1"
#define MQTT_USER DEVICE_ID
#define MQTT_PASS "version=2018-10-31&res=products%2F1bUie6VQTe%2Fdevices%2F%0Ad1&et=1779197232&method=md5&sign=ZlCrAKl%2BJQDFCnXCy6bFIQ%3D%3D"

// UART 文件描述符
extern int uart_fd;

// 函数声明
void uart_init(void);
void wifi_connect(void);
void mqtt_connect(void);
void send_mqtt_data(int zidongmoshi, int derection, int temp);
void esp_send_at(const char *at_cmd, const char *expected_resp);

#endif // WIFI_H
