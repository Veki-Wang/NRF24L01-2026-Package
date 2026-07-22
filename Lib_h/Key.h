#ifndef __KEY_H
#define __KEY_H

#include "stm32G4xx_hal.h"

// 引脚定义：和你CubeMX配置完全对应
#define KEY_PAGE_PORT    GPIOB
#define KEY_PAGE_PIN     GPIO_PIN_11   // KEY1：切换参数项

#define KEY_STEP_PORT    GPIOB
#define KEY_STEP_PIN     GPIO_PIN_12   // KEY2：切换调节步进

unsigned char Key_GetCode(void);
unsigned char Key_Get(void);
void Key_LoopDetect(void);
void key_task(void);

extern int16_t cnt;

#endif