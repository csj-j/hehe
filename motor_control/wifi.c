#include "wifi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

// UART 文件描述符
int uart_fd;

// 初始化 UART
void uart_init() {
    uart_fd = open("/dev/ttyS1", O_RDWR | O_NOCTTY | O_NDELAY);
    if (uart_fd < 0) {
        perror("Error opening UART");
        // 尝试备用串口
        uart_fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY);
        if (uart_fd < 0) {
            fprintf(stderr, "无法打开串口设备，请检查：\n");
            fprintf(stderr, "1. 设备是否存在 (ls /dev/ttyS*)\n");
            fprintf(stderr, "2. 用户权限 (groups | grep dialout)\n");
            fprintf(stderr, "3. 端口是否被占用 (lsof /dev/ttyS*)\n");
            exit(1);
        }
    }
    // ... 配置串口参数 ...
    struct termios options;
    tcgetattr(uart_fd, &options);
    options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    options.c_iflag = IGNPAR;
    options.c_oflag = 0;
    options.c_lflag = 0;
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &options);
}
    

// 发送 AT 命令并检查响应
void esp_send_at(const char *at_cmd, const char *expected_resp) {
    write(uart_fd, at_cmd, strlen(at_cmd));
    usleep(300000); // Wait for AT command to send
    
    char resp[256];
    int n = read(uart_fd, resp, sizeof(resp)-1);
    resp[n] = '\0';
    printf("AT Response: %s\n", resp);
 if (expected_resp) {
        int found = 0;
        char *token = strtok(resp, "\r\n");
        while (token != NULL) {
            if (strstr(token, expected_resp)) {
                found = 1;
                break;
            }
            token = strtok(NULL, "\r\n");
        }
        if (!found) {
            printf("Error: Expected '%s' not found in response\n", expected_resp);
            exit(1);
        }
    }
}

// 修正后的 wifi_connect 函数
void wifi_connect() {
    esp_send_at("AT+RST\r\n","ready"); // 修正拼写错误
    usleep(8000000);
    esp_send_at("AT+CWMODE=1\r\n", "OK");
    esp_send_at("AT+CWDHCP=1,1\r\n", "OK");
    usleep(1000000);
    esp_send_at("AT+CWJAP=\"HONOR 80\",\"liyou081909\"\r\n", "WIFI GOT IP");
    usleep(5000000); // 延长等待时间确保连接
}

// MQTT连接配置
void mqtt_connect() {
    char buf[512];
    snprintf(buf, sizeof(buf), 
             "AT+MQTTUSERCFG=0,1,\"%s\",0,0,\"\",0,1,\"%s\",0,0,0,0,0\r\n",
             DEVICE_ID, MQTT_PASS);
    esp_send_at(buf, "OK");
    
    snprintf(buf, sizeof(buf), 
             "AT+MQTTCONN=0,\"%s\",%d,1\r\n", MQTT_SERVER, MQTT_PORT);
    esp_send_at(buf, "+MQTTCONN: 0,0"); // 预期返回连接成功
    usleep(1000000);
}

// 发送 MQTT 数据
void send_mqtt_data(int zidongmoshi, int derection, int temp) {
    char json[512];
    snprintf(json, sizeof(json),
         "{\"id\":123,\"version\":\"1.0\",\"params\":{"
         "\"zidongmoshi\":%d,\"derection\":%d,\"temp\":%d}}", // 正确转义
         zidongmoshi, derection, temp);
    char buf[512];
    snprintf(buf, sizeof(buf), 
         "AT+MQTTPUB=0,\"$sys/%s/%s/thing/property/post\",\"%s\",1,0\r\n",
         PRODUCT_ID, DEVICE_ID, json);
    esp_send_at(buf, "+MQTTPUB: 0");
}
