#include "motor1.h"
#include "wifi.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    motor_init();
    
    // 初始化并连接网络
    uart_init();
    wifi_connect();
    mqtt_connect();

    int zidongmoshi = 1;  // 自动模式状态
    int derection = 0;    // 方向状态
    int temp = 0;         // 温度模拟值

    while (1) {
        // 自动模式逻辑
        if (zidongmoshi) {
            derection = (derection + 1) % 4;
            temp = (temp + 1) % 100;

             motor_forward();
             sleep(5);
             motor_right();
            sleep(5);
              motor_backward();
              sleep(5);
              motor_left();
              sleep(4);
              motor_forward();
              sleep(4);
              motor_right();
              sleep(3);
              motor_backward();
              sleep(3);
                motor_left();
               sleep(2);
                motor_stop();

        }
        // 手动模式
        else {
            motor_forward();
            motor_backward();
            motor_left();
            motor_right();
            motor_stop();
        }

        
        // 发送数据
        send_mqtt_data(zidongmoshi, derection, temp);
        
        // 模拟自动/手动模式切换
        zidongmoshi = !zidongmoshi;
        
        // 等待 5 秒后发送下一条数据
        sleep(5);
    }

    close(uart_fd);
    return 0;
}
