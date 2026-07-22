#include "Key.h"

static volatile unsigned char Key_Code = 0;

/**
 * @brief 外部调用函数：获取非连续键值（松手后返回一次）
 * @return unsigned char 按键键值（0=无按键，1=KEY1，2=KEY2）
 */
unsigned char Key_GetCode(void)
{
    unsigned char tempCode = Key_Code;
    Key_Code = 0;
    return tempCode;
}
/**
 * @brief 内部调用函数：获取当前按键状态（连续）
 * @return unsigned char 按键键值（0=无按键，1=KEY1，2=KEY2）
 */
unsigned char Key_Get(void)
{
    unsigned char currentKey = 0;

    if (HAL_GPIO_ReadPin(KEY_PAGE_PORT, KEY_PAGE_PIN) == GPIO_PIN_RESET)
    {
        currentKey = 1;
    }
    if (HAL_GPIO_ReadPin(KEY_STEP_PORT, KEY_STEP_PIN) == GPIO_PIN_RESET)
    {
        currentKey = 2;
    }

    return currentKey;
}

/**
 * @brief 检测按键状态（连续）
 */
void Key_LoopDetect(void)
{
    static unsigned char last = 0;
    unsigned char now = Key_Get();

    if (last != 0 && now == 0)
    {
        Key_Code = last;
    }

    last = now;
}

void key_task(void)
{
    Key_LoopDetect();
}
