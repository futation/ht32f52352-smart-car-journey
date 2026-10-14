#include "MPU6050.h"
#include "MPU6050_Reg.h"
#include "I2C1.h"
#include "stdio.h"

// GPIO配置
#define MPU6050_I2C_GPIO_PORT    HT_GPIOB
#define MPU6050_SCL_PIN          GPIO_PIN_7
#define MPU6050_SDA_PIN          GPIO_PIN_8

void MPU6050_Init(void)
{
    // 初始化I2C外设
    I2C1_Configuration();
  
    // MPU6050初始化序列
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);   // 退出睡眠模式
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);   // 所有传感器使能
    MPU6050_WriteReg(MPU6050_CONFIG, 0x03);       // 设置DLPF带宽
    MPU6050_WriteReg(MPU6050_SMPLRT_DIV, 0x09);   // 采样率分频
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18);  // 陀螺仪±2000dps
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18); // 加速度±16g
}

void MPU6050_WriteReg(uint8_t RegAddress, uint8_t Data)
{
    uint8_t buffer[2] = {RegAddress, Data};
    I2C1_write(MPU6050_ADDRESS, buffer, 2);
    I2C1_write_waiting();
}

uint8_t MPU6050_ReadReg(uint8_t RegAddress)
{
    uint8_t data;
    I2C1_read(MPU6050_ADDRESS, RegAddress, &data, 1);
    I2C1_read_waiting();
    return data;
}

uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                    int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    uint8_t buffer[14]; // 一次性读取14个寄存器（6轴数据 + 温度）
    I2C1_read(MPU6050_ADDRESS, MPU6050_ACCEL_XOUT_H, buffer, 14);
    I2C1_read_waiting();


    // 解析加速度数据
    *AccX = (buffer[0] << 8) | buffer[1];
    *AccY = (buffer[2] << 8) | buffer[3];
    *AccZ = (buffer[4] << 8) | buffer[5];
    
    // 温度数据（可选）
    // int16_t Temp = (buffer[6] << 8) | buffer[7];
    
    // 解析陀螺仪数据
    *GyroX = (buffer[8] << 8) | buffer[9];
    *GyroY = (buffer[10] << 8) | buffer[11];
    *GyroZ = (buffer[12] << 8) | buffer[13];
}
