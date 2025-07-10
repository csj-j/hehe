/* motor.c - 电机控制实现 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GPIO_BASE "/sys/class/gpio"
#define PWM_BASE "/sys/class/pwm/pwmchip1"

// GPIO控制函数实现
void gpio_export(int gpio) {
    FILE *fp = fopen(GPIO_BASE "/export", "w");
    if(fp) { 
        fprintf(fp, "%d", gpio); 
        fclose(fp); 
        usleep(100000);  // 等待系统创建节点
    } else {
        perror("GPIO export failed");
    }
}

void gpio_direction(int gpio, char *dir) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_BASE "/gpio%d/direction", gpio);
    FILE *fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "%s", dir); 
        fclose(fp);
    }
}

void gpio_set(int gpio, int value) {
    char path[50];
    snprintf(path, sizeof(path), GPIO_BASE "/gpio%d/value", gpio);
    FILE *fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "%d", value); 
        fclose(fp);
    }
}

// PWM控制函数实现
void pwm_export(int ch) {
    char path[50];
    snprintf(path, sizeof(path), PWM_BASE "/export");
    FILE *fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "%d", ch); 
        fclose(fp);
        usleep(100000);  // 等待系统创建节点
    }
}

void pwm_config(int ch, int period, int duty) {
    char path[50];
    
    // 设置周期
    snprintf(path, sizeof(path), PWM_BASE "/pwm%d/period", ch);
    FILE *fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "%d", period); 
        fclose(fp); 
    }
    
    // 设置占空比
    snprintf(path, sizeof(path), PWM_BASE "/pwm%d/duty_cycle", ch);
    fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "%d", duty); 
        fclose(fp);
    }
    
    // 使能PWM
    snprintf(path, sizeof(path), PWM_BASE "/pwm%d/enable", ch);
    fp = fopen(path, "w");
    if(fp) { 
        fprintf(fp, "1"); 
        fclose(fp);
    }
}

// 电机动作组合实现
void motor_stop(void) {
    gpio_set(44, 0); gpio_set(45, 0);
    gpio_set(50, 0); gpio_set(51, 0);
    printf("Motor: STOP\n");
}

void motor_forward(void) {
   
    gpio_set(44, 1); gpio_set(45, 0);  // 电机A正转
    gpio_set(50, 1); gpio_set(51, 0);  // 电机B正转
    printf("Motor: FORWARD\n");
}

void motor_backward(void) {
   
    gpio_set(44, 0); gpio_set(45, 1);  // 电机A反转
    gpio_set(50, 0); gpio_set(51, 1);  // 电机B反转
    printf("Motor: BACKWARD\n");
}

void motor_left(void) {
   
    gpio_set(44, 0); gpio_set(45, 1);  // 电机A反转
    gpio_set(50, 1); gpio_set(51, 0);  // 电机B正转
    printf("Motor: LEFT\n");
}

void motor_right(void) {
   
    gpio_set(44, 1); gpio_set(45, 0);  // 电机A正转
    gpio_set(50, 0); gpio_set(51, 1);  // 电机B反转
    printf("Motor: RIGHT\n");
}

// 电机系统初始化
void motor_init(void) {
    int gpios[] = {44, 45, 50, 51};
    
    printf("Initializing motor system...\n");
    for(int i = 0; i < 4; i++) {
        gpio_export(gpios[i]);
        gpio_direction(gpios[i], "out");
    }

    pwm_export(0); pwm_export(1);
    pwm_config(0, 10000, 4000);  // PWM1: 1MHz周期，50%占空比
    pwm_config(1, 10000, 4000);  // PWM2: 1MHz周期，50%占空比
    
    printf("Motor system initialized successfully\n");
}
int main() {
    // 初始化电机系统
    motor_init();
    
    // 控制序列
    while (1) {
        motor_forward();
        sleep(5);
        motor_left();
        usleep(500000);  // 0.5秒
        
        motor_forward();
        sleep(5);
        motor_left();
        usleep(500000);
        
        motor_forward();
        sleep(5);
        motor_right();
        usleep(500000);
        
        motor_forward();
        sleep(5);
        motor_right();
        usleep(500000);
        
        motor_forward();
        sleep(5);
        motor_left();
        usleep(1000000);
        
        motor_forward();
        sleep(5);
        motor_right();
        usleep(500000);
        
        motor_forward();
        sleep(5);
        motor_right();
        usleep(1000000);
        
        motor_forward();
        sleep(5);
        motor_left();
        usleep(500000);
        motor_stop();

    }
    
    return 0;
}


