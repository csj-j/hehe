/* encoder.c - 编码器实现 */
#include "encoder.h"       // 包含自定义编码器头文件
#include <stdio.h>         // 标准输入输出函数（printf, fprintf等）
#include <stdlib.h>        // 标准库函数（exit, malloc等）
#include <fcntl.h>         // 文件控制选项（O_RDONLY, O_NONBLOCK等）
#include <unistd.h>        // POSIX系统调用（read, write, usleep等）
#include <pthread.h>       // POSIX线程库
#include <sys/time.h>      // 时间相关函数（gettimeofday）
#include <poll.h>          // I/O多路复用（poll函数）
#include <string.h>        // 字符串操作函数
#include <math.h>

// GPIO定义 (龙芯2K1000LA)
#define GPIO_BASE "/sys/class/gpio"  // GPIO设备的系统路径
#define ENCODER_PINS 4               // 编码器使用的GPIO引脚数量（4个引脚）

// 编码器参数配置
const int encoder_pins[ENCODER_PINS] = {1, 2, 3, 4};  // GPIO引脚
const int ENCODER_PPR = 1040;           // 编码器每转脉冲数 (500线)
const float WHEEL_DIAMETER = 6.5;      // 轮子直径(厘米)
const float GEAR_RATIO = 20.0;          // 齿轮减速比 (1:1)

// 计算轮子周长 (厘米)
const float WHEEL_CIRCUMFERENCE = 3.14 * WHEEL_DIAMETER;

// 文件描述符数组，用于访问每个GPIO的value文件
int encoder_fds[ENCODER_PINS] = {0};

// 速度计数和互斥锁
volatile int pulse_count[2] = {0, 0};  // 两个电机的脉冲计数（0:左电机，1:右电机）
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;  // 保护计数器的互斥锁

// 上次读取时间（用于计算速度）
struct timeval last_read_time[2];  // 每个电机的时间戳

// GPIO初始化函数：导出GPIO引脚
static void gpio_export(int gpio) {
    // 打开export文件
    FILE *fp = fopen(GPIO_BASE "/export", "w");
    if(fp) { 
        // 写入GPIO编号以导出引脚
        fprintf(fp, "%d", gpio); 
        fclose(fp); 
        // 等待系统创建节点（100ms）
        usleep(100000);  
    } else {
        // 导出失败时打印错误信息
        perror("GPIO export failed");
    }
}

// GPIO初始化函数：设置引脚方向
static void gpio_direction(int gpio, const char *dir) {
    char path[50];  // 存储路径的缓冲区
    // 构建方向文件路径（如/sys/class/gpio/gpio1/direction）
    snprintf(path, sizeof(path), GPIO_BASE "/gpio%d/direction", gpio);
    FILE *fp = fopen(path, "w");
    if(fp) { 
        // 设置方向（输入"in"或输出"out"）
        fprintf(fp, "%s", dir); 
        fclose(fp);
    }
}

// GPIO初始化函数：配置边沿触发并打开值文件
static int gpio_open_edge(int gpio) {
    char path[50];  // 存储路径的缓冲区
    
    // 1. 配置边沿触发
    snprintf(path, sizeof(path), GPIO_BASE "/gpio%d/edge", gpio);
    FILE *fp = fopen(path, "w");
    if(fp) { 
        // 设置为双边沿触发（上升沿和下降沿都触发）
        fprintf(fp, "both");  
        fclose(fp);
    }
    
    // 2. 打开值文件用于读取
    snprintf(path, sizeof(path), GPIO_BASE "/gpio%d/value", gpio);
    // 以只读+非阻塞模式打开文件
    return open(path, O_RDONLY | O_NONBLOCK);
}

// 编码器监控线程函数（持续监控GPIO状态变化）
void* encoder_monitor(void* arg) {
    struct pollfd fds[ENCODER_PINS];  // poll结构数组
    char buf[16];  // 读取GPIO值的缓冲区
    
    // 初始化poll结构
    for (int i = 0; i < ENCODER_PINS; i++) {
        fds[i].fd = encoder_fds[i];      // 设置文件描述符
        fds[i].events = POLLPRI;         // 监听紧急数据（GPIO状态变化）
        
        // 重置文件指针到开头
        lseek(fds[i].fd, 0, SEEK_SET);  
        // 读取当前值（清除初始状态）
        read(fds[i].fd, buf, sizeof(buf));  
    }
    
    // 主监控循环
    while (1) {
        // 等待GPIO状态变化（无限期等待）
        if (poll(fds, ENCODER_PINS, -1) > 0) {
            // 检查所有GPIO
            for (int i = 0; i < ENCODER_PINS; i++) {
                // 检查是否有紧急数据（状态变化）
                if (fds[i].revents & POLLPRI) {
                    // 重置文件指针到开头
                    lseek(fds[i].fd, 0, SEEK_SET);
                    // 读取当前值（虽然值不重要，但需要读取来清除事件）
                    read(fds[i].fd, buf, sizeof(buf));
                    
                    // 确定电机ID (0或1)
                    // 引脚0和1 -> 电机0（左电机）
                    // 引脚2和3 -> 电机1（右电机）
                    int motor_id = i / 2;
                    
                    // 加锁保护计数器
                    pthread_mutex_lock(&count_mutex);
                    // 增加对应电机的脉冲计数
                    pulse_count[motor_id]++;
                    // 解锁
                    pthread_mutex_unlock(&count_mutex);
                }
            }
        }
    }
    return NULL;  // 线程不会返回
}

// 初始化编码器系统
void encoder_init(void) {
    printf("Initializing encoder system...\n");
    
    // 初始化所有GPIO引脚
    for (int i = 0; i < ENCODER_PINS; i++) {
        // 1. 导出GPIO引脚
        gpio_export(encoder_pins[i]);
        // 2. 设置为输入模式
        gpio_direction(encoder_pins[i], "in");
        // 3. 配置边沿触发并打开值文件
        encoder_fds[i] = gpio_open_edge(encoder_pins[i]);
        
        // 检查文件是否成功打开
        if (encoder_fds[i] < 0) {
            perror("Failed to open encoder GPIO");
            exit(1);  // 初始化失败退出程序
        }
    }
    
    // 初始化时间戳（记录当前时间）
    gettimeofday(&last_read_time[0], NULL);  // 左电机
    gettimeofday(&last_read_time[1], NULL);  // 右电机
    
    // 创建监控线程
    pthread_t monitor_thread;
    if (pthread_create(&monitor_thread, NULL, encoder_monitor, NULL)) {
        perror("Failed to create encoder thread");
        exit(1);  // 线程创建失败退出程序
    }
    
    // 分离线程，使其在退出时自动释放资源
    pthread_detach(monitor_thread);
    
    printf("Encoder system initialized\n");
}

float get_motor_speed_cms(int motor_id) {
    if (motor_id < 0 || motor_id > 1) return 0.0f;
    
    struct timeval now;
    gettimeofday(&now, NULL);
    
    int count = 0;
    pthread_mutex_lock(&count_mutex);
    count = pulse_count[motor_id];
    pulse_count[motor_id] = 0;
    pthread_mutex_unlock(&count_mutex);
    
    // 计算时间差（秒）
    long seconds = now.tv_sec - last_read_time[motor_id].tv_sec;
    long useconds = now.tv_usec - last_read_time[motor_id].tv_usec;
    float delta = seconds + useconds / 1000000.0f;
    
    // 更新时间戳
    last_read_time[motor_id] = now;
    
    if (delta <= 0) return 0.0f;
    
    // 计算实际线速度（厘米/秒）
    // 公式：速度 = (脉冲数 / 时间) × (轮子周长 / (编码器PPR × 4 × 减速比))
    // 其中 ×4 是因为使用了四倍频计数（双边沿检测）
    float pps = count / delta;  // 脉冲每秒
    float speed_cms = pps * (WHEEL_CIRCUMFERENCE / (ENCODER_PPR * 4 * GEAR_RATIO));
    
    return speed_cms;
}
