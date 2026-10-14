#ifndef __I2C1_H
#define __I2C1_H

//-----------------------------------------------------------------------------
#include "ht32.h"

//-----------------------------------------------------------------------------
typedef enum
{
  I2C1_UNFINISHED = 0,
  I2C1_FINISHED,
  I2C1_NACK,
  I2C1_TIMEOUT,
  I2C1_BUSERR,
} I2C1_RET_STATUS;

//-----------------------------------------------------------------------------
typedef struct 
{
  u8 *ptx_buffer;
  u16 tx_len;
  u16 tx_index;
  volatile I2C1_RET_STATUS tx_state;
  
  u8 *prx_buffer;
  u16 rx_len;
  u16 rx_index;
  volatile I2C1_RET_STATUS rx_state;
} I2C1_TypeDef;


//-----------------------------------------------------------------------------
#define I2C1_SCL_GPIO_PORT      HT_GPIOB       // GPIO端口 (例: HT_GPIOB)
#define I2C1_SCL_PIN            GPIO_PIN_7     // SCL引脚号 (例: GPIO_PIN_7)
#define I2C1_SDA_GPIO_PORT      HT_GPIOB       // GPIO端口 (例: HT_GPIOB)
#define I2C1_SDA_PIN            GPIO_PIN_8     // SDA引脚号 (例: GPIO_PIN_8)

#define I2C1_ADDRESS            0x55           // 主模式无需关注
#define I2C1_SPEED              100000         // I2C速度 (100kHz)

// 其他配置选项
#define ENABLE_I2C1_TIMEOUT
#define I2C1_TIMEOUT_COUNTER    (u32)(1000000/I2C1_SPEED) + 5   // 超时时间(ms)
// #define TWO_BYTE_REG_ADDRESS    // 若从设备需要2字节寄存器地址则开启
// #define ENABLE_INTERNAL_PULLUP  // 若需要内部上拉则开启

//-----------------------------------------------------------------------------
void I2C1_Configuration(void);
void I2C1_initial(void);
void I2C1_write(u8 dev_addr, u8 *dat, u16 len);
void I2C1_read(u8 dev_addr, u16 reg_addr, u8 *dat, u16 len);
void I2C1_read_directly(u8 dev_addr, u8 *dat, u16 len);
void I2C1_write_waiting(void);
void I2C1_read_waiting(void);
void I2C1_check_lock(void);
void I2C1_unlock(void);
void I2C1_err_detect(void);
void I2C1_test(void);

//-----------------------------------------------------------------------------

#endif

