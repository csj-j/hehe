/* motor.h - 电机控制头文件 */
#ifndef MOTOR1_H
#define MOTOR1_H

// GPIO控制函数
void gpio_export(int gpio);
void gpio_direction(int gpio, char *dir);
void gpio_set(int gpio, int value);

// PWM控制函数
void pwm_export(int ch);
void pwm_config(int ch, int period, int duty);

// 电机动作控制
void motor_stop(void);
void motor_forward(void);
void motor_backward(void);
void motor_left(void);
void motor_right(void);
void motor_init(void);

#endif // MOTOR_H
