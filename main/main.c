#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "driver/i2s.h"

// I2S 引脚定义
#define I2S_NUM         I2S_NUM_0   // 使用 I2S0
#define I2S_BCLK_PIN    19          // BCLK 引脚，即Bit Clock，时钟信号
#define I2S_WS_PIN      20          // LRC 引脚，即Word Select，数据信号
#define I2S_DO_PIN      18          // DIN 引脚，即Data Input，输入信号
#define I2S_DI_PIN      -1          // 不使用输入信号，设置为 -1

// 音频配置
#define SAMPLE_RATE     44100       // 采样率（44.1kHz）
#define SAMPLE_BITS     I2S_BITS_PER_SAMPLE_16BIT // 每样本 16 位
#define DMA_BUF_COUNT   4           // DMA 缓冲区数量
#define DMA_BUF_LEN     1024        // 每个缓冲区的大小

// 初始化 I2S
void i2s_init() {
    // 配置 I2S
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,  // 主机模式，只发送数据
        .sample_rate = SAMPLE_RATE,             // 采样率
        .bits_per_sample = SAMPLE_BITS,         // 每样本位数
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // 立体声
        .communication_format = I2S_COMM_FORMAT_I2S,  // 标准 I2S 格式
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,     // 中断优先级
        .dma_buf_count = DMA_BUF_COUNT, // DMA 缓冲区数量
        .dma_buf_len = DMA_BUF_LEN,     // 每个缓冲区的大小
        .use_apll = false,              // 不使用 APLL，APLL即Audio PLL，音频锁相环
        .tx_desc_auto_clear = true,  // 自动清空 TX 描述符
    };

    // 配置 I2S 引脚
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK_PIN,  // BCLK 引脚
        .ws_io_num = I2S_WS_PIN,     // LRC 引脚
        .data_out_num = I2S_DO_PIN,  // DIN 引脚
        .data_in_num = I2S_DI_PIN    // 不使用输入
    };

    // 安装和启动 I2S 驱动
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);
}

// 定义音符结构体
typedef struct {
    int frequency;    // 频率
    int duration_ms;  // 持续时间（毫秒）
} note_t;

// 《小星星》旋律音符
const note_t melody[] = {
    { 261, 500 }, // C
    { 261, 500 }, // C
    { 392, 500 }, // G
    { 392, 500 }, // G
    { 440, 500 }, // A
    { 440, 500 }, // A
    { 392, 1000 },// G
    { 349, 500 }, // F
    { 349, 500 }, // F
    { 329, 500 }, // E
    { 329, 500 }, // E
    { 294, 500 }, // D
    { 294, 500 }, // D
    { 261, 1000 } // C
};
const int melody_length = sizeof(melody) / sizeof(melody[0]);

// 播放单个音符
void play_note(int frequency, int duration_ms) {
    const int amplitude = 3000;     // 振幅，即音量
    int samples_per_cycle = SAMPLE_RATE / frequency;    // 每个周期的采样数，即波长
    int16_t sine_wave[samples_per_cycle * 2];        // 正弦波数据，左右声道

    // 生成单个音符的正弦波数据
    for (int i = 0; i < samples_per_cycle; i++) {
        int16_t sample_value = (int16_t)(amplitude * sinf(2 * M_PI * i / samples_per_cycle));
        sine_wave[2 * i]     = sample_value;  // 左声道
        sine_wave[2 * i + 1] = sample_value;  // 右声道
    }

    int loops = (duration_ms * SAMPLE_RATE) / (samples_per_cycle * 1000);   // 循环次数
    size_t bytes_written;
    for (int i = 0; i < loops; i++) {
        i2s_write(I2S_NUM, sine_wave, sizeof(sine_wave), &bytes_written, portMAX_DELAY);    // 写入 I2S
    }
}

// 播放《小星星》旋律
void play_melody() {
    for (int i = 0; i < melody_length; i++) {
        play_note(melody[i].frequency, melody[i].duration_ms);
        vTaskDelay(pdMS_TO_TICKS(100)); // 每个音符间隔100ms
    }
}

void app_main(void) {
    printf("MAX98357A I2S Audio Example\n");

    // 初始化 I2S
    i2s_init();

    // 播放《小星星》旋律
    play_melody();
}