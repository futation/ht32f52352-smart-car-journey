/* [file name]: MPU6050_HT32.c */
#include "MPU6050.h"
#include "MPU6050_Reg.h"
#include "delay.h"

/*----- I2C基础函数 -----*/
void MPU6050_I2C_Init(void)
{
    // 使能GPIOB时钟
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.PB = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // 配置SCL和SDA为开漏输出
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SCL_PIN, GPIO_DIR_OUT);
    GPIO_OpenDrainConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SCL_PIN, ENABLE);
    
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, GPIO_DIR_OUT);
    GPIO_OpenDrainConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, ENABLE);

    // 初始拉高总线
    MPU6050_SCL_H();
    MPU6050_SDA_H();
}

static void I2C_Start(void)
{
    MPU6050_SDA_H();
    MPU6050_SCL_H();
    Delay_us(5);
    MPU6050_SDA_L();
    Delay_us(5);
    MPU6050_SCL_L();
}

static void I2C_Stop(void)
{
    MPU6050_SDA_L();
    MPU6050_SCL_H();
    Delay_us(5);
    MPU6050_SDA_H();
    Delay_us(5);
}

static uint8_t I2C_WaitAck(void)
{
    uint8_t ack;
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, GPIO_DIR_IN);
    MPU6050_SCL_H();
    Delay_us(5);
    ack = MPU6050_SDA_READ();
    MPU6050_SCL_L();
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, GPIO_DIR_OUT);
    return (ack == 0) ? 0 : 1; // 0=ACK成功
}

// 修改I2C_SendByte函数，增加ACK错误处理
static void I2C_SendByte(uint8_t byte)
{
    for(uint8_t i=0; i<8; i++){
        if(byte & 0x80) MPU6050_SDA_H();
        else MPU6050_SDA_L();
        byte <<= 1;
        Delay_us(5);  // 增加延时确保信号稳定
        MPU6050_SCL_H();
        Delay_us(10); // SCL高电平保持时间延长
        MPU6050_SCL_L();
        Delay_us(5);
    }
    if(I2C_WaitAck() != 0){
        // 若ACK失败，直接终止通信
        I2C_Stop();
        return;
    }
}

static uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t byte = 0;
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, GPIO_DIR_IN);
    
    for(uint8_t i=0; i<8; i++){
        byte <<= 1;
        MPU6050_SCL_H();
        Delay_us(5);
        if(MPU6050_SDA_READ()) byte |= 0x01;
        MPU6050_SCL_L();
        Delay_us(2);
    }
    
    GPIO_DirectionConfig(MPU6050_I2C_GPIO_PORT, MPU6050_SDA_PIN, GPIO_DIR_OUT);
    if(ack) MPU6050_SDA_L();
    else MPU6050_SDA_H();
    
    MPU6050_SCL_H();
    Delay_us(5);
    MPU6050_SCL_L();
    return byte;
}

/*----- MPU6050功能函数 -----*/
void MPU6050_WriteReg(uint8_t reg, uint8_t data)
{
    I2C_Start();
    I2C_SendByte(MPU6050_ADDRESS);
    I2C_SendByte(reg);
    I2C_SendByte(data);
    I2C_Stop();
}

uint8_t MPU6050_ReadReg(uint8_t reg)
{
    uint8_t data;
    I2C_Start();
    I2C_SendByte(MPU6050_ADDRESS);
    I2C_SendByte(reg);
    I2C_Start(); // 重复起始条件
    I2C_SendByte(MPU6050_ADDRESS | 0x01);
    data = I2C_ReadByte(0);
    I2C_Stop();
    return data;
}

void MPU6050_Init(void)
{
    MPU6050_I2C_Init();
    Delay_ms(100);
    
    MPU6050_WriteReg(MPU6050_PWR_MGMT_1, 0x01);  // 唤醒设备
    MPU6050_WriteReg(MPU6050_PWR_MGMT_2, 0x00);  // 启用所有传感器
    MPU6050_WriteReg(MPU6050_CONFIG, 0x06);      // 低通滤波
    MPU6050_WriteReg(MPU6050_GYRO_CONFIG, 0x18); // ±2000dps
    MPU6050_WriteReg(MPU6050_ACCEL_CONFIG, 0x18);// ±16g
    Delay_ms(50);
}

uint8_t MPU6050_GetID(void)
{
    return MPU6050_ReadReg(MPU6050_WHO_AM_I);
}

void MPU6050_GetData(int16_t *AccX, int16_t *AccY, int16_t *AccZ,
                   int16_t *GyroX, int16_t *GyroY, int16_t *GyroZ)
{
    I2C_Start();
    I2C_SendByte(MPU6050_ADDRESS);
    I2C_SendByte(MPU6050_ACCEL_XOUT_H);
    I2C_Start(); // 重复起始条件
    I2C_SendByte(MPU6050_ADDRESS | 0x01);
    
    // 连续读取14字节数据（加速度+温度+陀螺仪）
    *AccX  = (I2C_ReadByte(1) << 8) | I2C_ReadByte(1);
    *AccY  = (I2C_ReadByte(1) << 8) | I2C_ReadByte(1);
    *AccZ  = (I2C_ReadByte(1) << 8) | I2C_ReadByte(1);
    I2C_ReadByte(1); I2C_ReadByte(1); // 跳过温度
    *GyroX = (I2C_ReadByte(1) << 8) | I2C_ReadByte(1);
    *GyroY = (I2C_ReadByte(1) << 8) | I2C_ReadByte(1);
    *GyroZ = (I2C_ReadByte(0) << 8) | I2C_ReadByte(0);
    
    I2C_Stop();
}