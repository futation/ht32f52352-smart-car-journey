/* [file name]: MPU6050_HT32.h */
#ifndef __MPU6050_H
#define __MPU6050_H

#include "ht32.h"

// 引脚定义（根据用户要求配置PB7/PB8）
#define MPU6050_I2C_GPIO_PORT    HT_GPIOB
#define MPU6050_SCL_PIN          GPIO_PIN_7
#define MPU6050_SDA_PIN          GPIO_PIN_8

// 引脚操作宏（参考OLED驱动实现）
#define MPU6050_SCL_H()    GPIO_SetOutBits(MPU6050_I2C_GPIO_PORT, MPU6050_SCL_PIN)
#define MPU6050_SCL_L()    GPIO_ClearOutBits(MPU6050_I2C_GPIO_PORT, MPU6050_SCL_PIN)
#define MPU6050_SDA_H()    GPIO_SetOutBits(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN)
#define MPU6050_SDA_L()    GPIO_ClearOutBits(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN)
#define MPU6050_SDA_READ() GPIO_ReadInBit(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN)

// MPU6050地址
#define MPU6050_ADDRESS     0xD0

void MPU6050_I2C_Init(void);
void MPU6050_WriteReg(uint8_t reg, uint8_t data);
uint8_t MPU6050_ReadReg(uint8_t reg);
void MPU6050_Init(void);
uint8_t MPU6050_GetID(void);
void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                   int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ);

#endif