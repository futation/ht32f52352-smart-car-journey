#include "I2C1.h"

//-----------------------------------------------------------------------------
//                         全局变量定义
//-----------------------------------------------------------------------------
I2C1_TypeDef struct_i2c1;    // I2C1控制结构体，包含发送/接收缓冲区状态信息
vu16 i2c1_timeout_ct;         // I2C超时计数器，用于检测通信超时

//-----------------------------------------------------------------------------
//                         静态函数声明
//-----------------------------------------------------------------------------
/* 简单延时函数 */
static void clk_delay(u16 ct)
{
  while(ct--); // 通过空循环实现短暂延时
}

/* 配置目标设备地址和传输方向 */
static void I2C1_TargetAddressConfig(HT_I2C_TypeDef* I2Cx, u16 I2C_Address, u32 I2C_Direction)
{
  if (I2C_Direction != I2C_MASTER_WRITE) // 读模式
  {
    I2Cx->TAR = (I2C_Address << 1) | I2C_MASTER_READ; // 设置目标地址+读标志
  }
  else // 写模式
  {
    I2Cx->TAR = (I2C_Address << 1) | I2C_MASTER_WRITE; // 设置目标地址+写标志
  }
  i2c1_timeout_ct = 0; // 重置超时计数器
}

//-----------------------------------------------------------------------------
//                         主要功能函数
//-----------------------------------------------------------------------------
/* I2C1外设配置 */
void I2C1_Configuration(void)
{
  u16 delay_ct;
  
  // 启用GPIOB和I2C1时钟
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.I2C1 = 1;  // I2C1时钟使能
  CKCUClock.Bit.PB = 1;     // GPIOB时钟使能
  CKCUClock.Bit.AFIO = 1;   // 复用功能时钟使能
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  // 如果启用内部上拉（需要时取消注释ENABLE_INTERNAL_PULLUP）
//  #ifdef ENABLE_INTERNAL_PULLUP
  GPIO_PullResistorConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_PR_UP);  
  GPIO_PullResistorConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, GPIO_PR_UP);  
//  #endif
  
  // 配置PB7和PB8为I2C复用功能
  AFIO_GPxConfig(GPIO_PB, I2C1_SCL_PIN, AFIO_FUN_I2C);
  AFIO_GPxConfig(GPIO_PB, I2C1_SDA_PIN, AFIO_FUN_I2C);

  // 配置开漏模式（I2C标准要求）
  GPIO_OpenDrainConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, ENABLE);
  GPIO_OpenDrainConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, ENABLE);

  // 短暂延时确保配置生效
  for(delay_ct=0; delay_ct<256; delay_ct++);

  I2C1_check_lock(); // 检查总线是否被锁
  I2C1_initial();    // 初始化I2C外设
}
  

/* I2C1外设初始化 */
void I2C1_initial(void)
{
  I2C_InitTypeDef  I2C_InitStruct;
  RSTCU_PeripReset_TypeDef RSTCUReset = {{0}};
  RSTCUReset.Bit.I2C1 = 1;
  RSTCU_PeripReset(RSTCUReset, ENABLE);// 复位I2C1外设

  /* I2C配置参数 */
  /* I2C配置参数 */
  I2C_InitStruct.I2C_GeneralCall = I2C_GENERALCALL_DISABLE;   // 禁用广播呼叫
  I2C_InitStruct.I2C_AddressingMode = I2C_ADDRESSING_7BIT;     // 7位地址模式
  I2C_InitStruct.I2C_Acknowledge = I2C_ACK_ENABLE;             // 启用ACK响应
  I2C_InitStruct.I2C_OwnAddress = I2C1_ADDRESS;                // 本设备地址
  I2C_InitStruct.I2C_Speed = I2C1_SPEED;                      // 通信速率（100kHz）
  I2C_InitStruct.I2C_SpeedOffset = 0;                         // 速率偏移量
  I2C_Init(HT_I2C1, &I2C_InitStruct);                          // 应用配置
  I2C_Cmd(HT_I2C1, ENABLE);                                   // 使能I2C
//  I2C_SequentialFilterConfig(HT_I2C1, SEQ_FILTER_2_PCLK);  

  // 配置中断（包含状态、地址、NACK、总线错误等事件）
  I2C_IntConfig(HT_I2C1, I2C_INT_STA | I2C_INT_ADRS | I2C_INT_RXNACK | 
                   I2C_INT_BUSERR | I2C_INT_RXDNE | I2C_INT_TXDE, ENABLE);

  //NVIC_SetPriority(I2C1_IRQn, 1);
  NVIC_EnableIRQ(I2C1_IRQn); // 使能I2C1中断
  i2c1_timeout_ct = 0;        // 初始化超时计数器
}


//-----------------------------------------------------------------------------
//                         中断服务函数
//-----------------------------------------------------------------------------
/* I2C1中断处理函数（核心通信逻辑） */
void I2C1_IRQHandler(void)
{
  u32 status = I2C_ReadRegister(HT_I2C1, I2C_REGISTER_SR);
  if(status & I2C_FLAG_MASTER)    // 主模式处理
  {
    if(status & I2C_FLAG_STA)  // 检测到START信号
    {
      i2c1_timeout_ct = 0;
    }
    if(status & I2C_FLAG_MASTER)    // 主模式处理
    {
      if(status & I2C_FLAG_ADRS)  // 地址发送完成
      {
        I2C_SendData(HT_I2C1, struct_i2c1.ptx_buffer[struct_i2c1.tx_index++]);
        i2c1_timeout_ct = 0;
      }
      else if(status & I2C_FLAG_TXDE)
      {
        if (struct_i2c1.tx_index < struct_i2c1.tx_len)
          I2C_SendData(HT_I2C1, struct_i2c1.ptx_buffer[struct_i2c1.tx_index++]);  //Send the remainder data to I2C Slave
        else
        {
          I2C_GenerateSTOP(HT_I2C1);
          struct_i2c1.tx_state = I2C1_FINISHED;
        }
        i2c1_timeout_ct = 0;
      }
    }
    else  //rx mode
    {
      if(status & I2C_FLAG_ADRS)    //Master Receiver
      {
        if(struct_i2c1.rx_len <= 1)
          I2C_AckCmd(HT_I2C1, DISABLE);
        else
          I2C_AckCmd(HT_I2C1, ENABLE);
        i2c1_timeout_ct = 0;
      }
      else if(status & I2C_FLAG_RXDNE)
      {
        struct_i2c1.prx_buffer[struct_i2c1.rx_index++] = I2C_ReceiveData(HT_I2C1);
        if (struct_i2c1.rx_index == struct_i2c1.rx_len - 1)
        {
          I2C_AckCmd(HT_I2C1, DISABLE); // 倒数第二个字节后禁用ACK
        }
        if (struct_i2c1.rx_index == struct_i2c1.rx_len)
        {
          I2C_GenerateSTOP(HT_I2C1);    // 生成STOP信号
          struct_i2c1.rx_state = I2C1_FINISHED;
        }
        i2c1_timeout_ct = 0;
      }
    }
    if(status & (I2C_FLAG_BUSERR|I2C_FLAG_ARBLOS))
    {
      I2C_ClearFlag(HT_I2C1, I2C_FLAG_BUSERR|I2C_FLAG_ARBLOS);
      struct_i2c1.tx_state = I2C1_BUSERR;
      struct_i2c1.rx_state = I2C1_BUSERR;
    }
    if(status & I2C_FLAG_TOUTF)
    {
      I2C_ClearFlag(HT_I2C1, I2C_FLAG_TOUTF);
      struct_i2c1.tx_state = I2C1_TIMEOUT;
      struct_i2c1.rx_state = I2C1_TIMEOUT;
    }
    if(status & I2C_FLAG_RXNACK) 
    {
      I2C_ClearFlag(HT_I2C1, I2C_FLAG_RXNACK);
      I2C_GenerateSTOP(HT_I2C1);
      
      struct_i2c1.tx_state = I2C1_NACK;
      if(!struct_i2c1.rx_state)
        struct_i2c1.rx_state = I2C1_NACK;
    }
  }
  else
  {
    I2C_ClearFlag(HT_I2C1, status & 0x0f00);
  }
}

//-----------------------------------------------------------------------------
//                         通信接口函数
//-----------------------------------------------------------------------------
/* 启动I2C写操作 */
void I2C1_write(u8 dev_addr, u8 *dat, u16 len)
{
  struct_i2c1.ptx_buffer = dat;
  struct_i2c1.tx_len = len ? len : 1;
  struct_i2c1.tx_index = 0;
  struct_i2c1.tx_state = I2C1_UNFINISHED;
  I2C1_check_lock();
  I2C1_TargetAddressConfig(HT_I2C1, dev_addr, I2C_MASTER_WRITE);
}

/* 启动I2C读操作（带寄存器地址）*/
void I2C1_read(u8 dev_addr, u16 reg_addr, u8 *dat, u16 len)
{
  u8 addr_buf[2];
  
  #ifdef TWO_BYTE_REG_ADDRESS
    addr_buf[0] = (reg_addr>>8)&0xff;
    addr_buf[1] = reg_addr&0xff;
    I2C1_write(dev_addr, addr_buf, 2); 
  #else
    addr_buf[0] = reg_addr&0xff;
    I2C1_write(dev_addr, addr_buf, 1); 
  #endif
  
  while(struct_i2c1.tx_state == I2C1_UNFINISHED)
  {
    #ifdef ENABLE_I2C1_TIMEOUT
    if(i2c1_timeout_ct > I2C1_TIMEOUT_COUNTER)
    {
      I2C1_initial();
      break;
    }
    #endif
  }

  if(struct_i2c1.tx_state == I2C1_FINISHED)
  {
    struct_i2c1.prx_buffer = dat;
    struct_i2c1.rx_len = len ? len : 1;;
    struct_i2c1.rx_index = 0;
    struct_i2c1.rx_state = I2C1_UNFINISHED;
    I2C1_check_lock();
      
    // 添加ACK控制：如果读取长度大于1，启用ACK；否则禁用
    if (len > 1) 
    {
      I2C_AckCmd(HT_I2C1, ENABLE);
    } 
    else 
    {
      I2C_AckCmd(HT_I2C1, DISABLE);
    }

    I2C1_TargetAddressConfig(HT_I2C1, dev_addr, I2C_MASTER_READ);
  }
  else
  {
    struct_i2c1.rx_state = I2C1_NACK;
  }
}

//-----------------------------------------------------------------------------
// @brief:  Slave device maybe do not need to write address at first
//-----------------------------------------------------------------------------
void I2C1_read_directly(u8 dev_addr, u8 *dat, u16 len)
{
  struct_i2c1.prx_buffer = dat;
  struct_i2c1.rx_len = len ? len : 1;;
  struct_i2c1.rx_index = 0;
  struct_i2c1.rx_state = I2C1_UNFINISHED;
  I2C1_check_lock();
 
  I2C1_TargetAddressConfig(HT_I2C1, dev_addr, I2C_MASTER_READ);
}

//-----------------------------------------------------------------------------
void I2C1_write_waiting(void)
{
  while(struct_i2c1.tx_state == I2C1_UNFINISHED)
  {
    #ifdef ENABLE_I2C1_TIMEOUT
    if(i2c1_timeout_ct > I2C1_TIMEOUT_COUNTER)
    {
      I2C1_initial();
      break;
    }
    #endif
  }
}

//-----------------------------------------------------------------------------
void I2C1_read_waiting(void)
{
  while(struct_i2c1.rx_state == I2C1_UNFINISHED)
  {
    #ifdef ENABLE_I2C1_TIMEOUT
    if(i2c1_timeout_ct > I2C1_TIMEOUT_COUNTER)
    {
      I2C1_initial();
      break;
    }
    #endif
  }
}

//-----------------------------------------------------------------------------
void I2C1_check_lock(void)
{
  u32 ct;
  for(ct=0; ct<100; ct++) //waiting for stop end
  {
    if(!(HT_I2C1->CR & 0x02))
      break;
    else
      clk_delay(100);
  }
  if(GPIO_ReadInBit(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN) == RESET)
    I2C1_unlock();
}

//-----------------------------------------------------------------------------
// @brief   If slave has locked SDA, need unlock at first
//-----------------------------------------------------------------------------
void I2C1_unlock(void)
{
  u8 i;
  /* I2C configuration */
  AFIO_GPxConfig(GPIO_PB, I2C1_SCL_PIN, AFIO_FUN_DEFAULT);
  AFIO_GPxConfig(GPIO_PB, I2C1_SDA_PIN, AFIO_FUN_DEFAULT);
  GPIO_OpenDrainConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, ENABLE);
  GPIO_SetOutBits     (I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
  GPIO_DirectionConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_DIR_OUT);

  GPIO_OpenDrainConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, ENABLE);
  GPIO_SetOutBits     (I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN);
  GPIO_DirectionConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, GPIO_DIR_OUT);

  GPIO_ClearOutBits   (I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
  clk_delay(50);
  for(i=0; i<9; i++)
  {
    GPIO_SetOutBits   (I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
    clk_delay(50);
    GPIO_ClearOutBits (I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
    clk_delay(50);
  }
  GPIO_ClearOutBits   (I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN);
  clk_delay(50);
  GPIO_SetOutBits     (I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN);
  clk_delay(50);
  GPIO_SetOutBits     (I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN);

  GPIO_DirectionConfig(I2C1_SCL_GPIO_PORT, I2C1_SCL_PIN, GPIO_DIR_IN);
  GPIO_DirectionConfig(I2C1_SDA_GPIO_PORT, I2C1_SDA_PIN, GPIO_DIR_IN);
  AFIO_GPxConfig(GPIO_PB, I2C1_SCL_PIN, AFIO_FUN_I2C);
  AFIO_GPxConfig(GPIO_PB, I2C1_SDA_PIN, AFIO_FUN_I2C);
}

//-----------------------------------------------------------------------------
void I2C1_err_detect(void)
{
  __disable_irq();
  i2c1_timeout_ct++;
  __enable_irq();
}

//-----------------------------------------------------------------------------
#define TARGET_DEV_ADDR 0x50
//static  u8 read_addr = 0x00;
void I2C1_test(void)
{
  u8 i,test_array[4];
  for(i=0; i<4; i++)
  {
    test_array[i] = i;
  }
  I2C1_write(TARGET_DEV_ADDR, test_array, 4);    //write data test:device addr + register addr + data
  I2C1_write_waiting();

//  I2C1_write(TARGET_DEV_ADDR, &read_addr, 1); //The first read way: write address at first,then ...
//  I2C1_write_waiting();
  I2C1_read_directly(TARGET_DEV_ADDR, test_array, 4); //then read directly
  I2C1_read_waiting();                         //can process other task while waiting

//  I2C1_read(TARGET_DEV_ADDR, read_addr, test_array, 4); //The second read way: write reg address and read data back, as same as operate EEPROM
//  I2C1_read_waiting();
}


