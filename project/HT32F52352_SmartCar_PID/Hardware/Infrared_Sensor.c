/* Infrared_Sensor.c */
#include "Infrared_Sensor.h"
#include "ht32_board.h"

/**
  * @brief 红外传感器初始化
  */
void Infrared_Sensor_Init(void)
{
    /* 使能GPIOA时钟 */
	  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.PA = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);
    
    /* 配置左红外传感器引脚 */
    GPIO_DirectionConfig(LEFT_INFRARED_PORT, LEFT_INFRARED_PIN, GPIO_DIR_IN);
    GPIO_PullResistorConfig(LEFT_INFRARED_PORT, LEFT_INFRARED_PIN, GPIO_PR_UP);
    GPIO_InputConfig(LEFT_INFRARED_PORT, LEFT_INFRARED_PIN, ENABLE);
    
    /* 配置右红外传感器引脚 */
    GPIO_DirectionConfig(RIGHT_INFRARED_PORT, RIGHT_INFRARED_PIN, GPIO_DIR_IN);
    GPIO_PullResistorConfig(RIGHT_INFRARED_PORT, RIGHT_INFRARED_PIN, GPIO_PR_UP);
    GPIO_InputConfig(RIGHT_INFRARED_PORT, RIGHT_INFRARED_PIN, ENABLE);
}

/**
  * @brief 读取左红外传感器状态
  * @retval 0:检测到障碍物 1:未检测到
  */
uint8_t Read_Left_Infrared_Sensor(void)
{
    FlagStatus status = GPIO_ReadInBit(LEFT_INFRARED_PORT, LEFT_INFRARED_PIN);
    return (status == RESET) ? 0 : 1;
}

/**
  * @brief 读取右红外传感器状态
  * @retval 0:检测到障碍物 1:未检测到
  */
uint8_t Read_Right_Infrared_Sensor(void)
{
    FlagStatus status = GPIO_ReadInBit(RIGHT_INFRARED_PORT, RIGHT_INFRARED_PIN);
    return (status == RESET) ? 0 : 1;
}
