/* encoder.h - 编码器接口 */
#ifndef ENCODER_H
#define ENCODER_H

// 初始化编码器系统
void encoder_init(void);

// 获取电机速度（脉冲/秒）
float get_motor_speed_cms(int motor_id);

#endif // ENCODER_H
