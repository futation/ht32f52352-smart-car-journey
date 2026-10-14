#include "Scaner.h"  // 包含USART1相关定义（如_USART1_STRUCT结构体、常量等）

//-----------------------------------------------------------------------------
// 定义接收和发送缓冲区结构体实例（4字节对齐优化访问速度）
__ALIGN4 _USART1_STRUCT rxd_scomm1;  // 接收缓冲区（存储来自USART的数据）
__ALIGN4 _USART1_STRUCT txd_scomm1;  // 发送缓冲区（存储待发送至USART的数据）

uint8_t Serial_RxPacket1[SERIAL_RX_PACKET_SIZE];
volatile uint8_t Serial_RxFlag1= 0;

//-----------------------------------------------------------------------------
/**
  * @brief  USART1初始化配置（时钟、GPIO、工作模式、中断等）
  * @note   波特率9600, 8位数据, 无校验, 1停止位
  *         使用PB15(TX)、PC0(RX)作为USART1引脚
  */
void USART1_Configuration(void)
{
  USART_InitTypeDef USART_InitStruct;
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};

  // 使能外设时钟
  CKCUClock.Bit.USART1   = 1;  // USART1时钟
  CKCUClock.Bit.AFIO     = 1;  // 复用功能时钟
  CKCUClock.Bit.PB       = 1;  // GPIOB时钟 
  CKCUClock.Bit.PC       = 1;  // GPIOC时钟（用于RX引脚PC0）
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  // 配置RX引脚PC0上拉电阻（防止浮空输入干扰）
  GPIO_PullResistorConfig(HT_GPIOC, GPIO_PIN_0, GPIO_PR_UP);

  // 配置PB15为USART1_TX，PC0为USART1_RX（复用功能）
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_15, AFIO_FUN_USART_UART);
  AFIO_GPxConfig(GPIO_PC, AFIO_PIN_0, AFIO_FUN_USART_UART);

  // USART参数配置
  USART_InitStruct.USART_BaudRate = 9600;          // 波特率
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B;  // 8位数据
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;       // 1停止位
  USART_InitStruct.USART_Parity = USART_PARITY_NO;          // 无校验
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;          // 普通模式（非多机）
  USART_Init(HT_USART1, &USART_InitStruct);

  // 使能USART接收和发送功能
  USART_RxCmd(HT_USART1, ENABLE);
  USART_TxCmd(HT_USART1, ENABLE);

//  // 配置FIFO阈值（触发中断条件）
//  USART_RXTLConfig(HT_USART1, USART_RXTL_04);  // RX FIFO >=4字节触发中断
//  USART_TXTLConfig(HT_USART1, USART_TXTL_04);  // TX FIFO <=4字节触发中断

  // 使能接收数据就绪中断和接收超时中断
  USART_IntConfig(HT_USART1, USART_INT_RXDR, ENABLE);

//  // 设置接收超时时间为40个波特周期（根据波特率计算实际超时时间）
//  HT_USART1->TPR = 0x80 | 40;  // 0x80启用超时功能，40为超时计数值

  // 使能USART1中断（NVIC级）
  NVIC_EnableIRQ(USART1_IRQn);

  // 初始化接收/发送缓冲区
  USART1_init_buffer();
}

//-----------------------------------------------------------------------------
/**
  * @brief  初始化USART1接收和发送缓冲区
  * @note   环形缓冲区结构：read_pt（读指针）、write_pt（写指针）、cnt（数据计数）
  */
void USART1_init_buffer (void)
{
  // 接收缓冲区初始化
  rxd_scomm1.read_pt = 0;   // 读指针归零
  rxd_scomm1.write_pt = 0;  // 写指针归零
  rxd_scomm1.cnt = 0;       // 数据计数器归零

  // 发送缓冲区初始化
  txd_scomm1.read_pt = 0;
  txd_scomm1.write_pt = 0;
  txd_scomm1.cnt = 0;
}


//-----------------------------------------------------------------------------
/**
  * @brief  解析接收缓冲区中的数据
  * @note   示例代码：检查数据头0xAA，并处理8字节完整帧
  * @note   实际应用中需根据协议修改解析逻辑
  */
void USART1_analyze_data(void)
{
    static uint8_t state = 0;
    static uint8_t index = 0;

    while (rxd_scomm1.cnt > 0)
    {
        uint8_t data = rxd_scomm1.buffer[rxd_scomm1.read_pt];
        rxd_scomm1.read_pt = (rxd_scomm1.read_pt + 1) % USART1_BUF_SIZE;
        NVIC_DisableIRQ(USART1_IRQn);
        rxd_scomm1.cnt--;
        NVIC_EnableIRQ(USART1_IRQn);

        switch (state)
        {
        case 0: // 等待包头'@'
            if (data == '@')
            {
                Serial_RxPacket1[0] = data;
                index = 1;
                state = 1;
            }
            break;

        case 1: // 接收数据直到包尾'.'
            if (data == '.')
            {
                Serial_RxPacket1[index] = '\0';
                Serial_RxFlag1 = 1;
                state = 0;
            }
            else if (index < SERIAL_RX_PACKET_SIZE - 1)
            {
                Serial_RxPacket1[index++] = data;
            }
            else
            {
                // 缓冲区溢出处理
                state = 0;
                Serial_RxFlag1 = 0xFF;
            }
            break;
        }
    }
}

//-----------------------------------------------------------------------------
/**
  * @brief  将数据写入发送缓冲区并触发发送
  * @param  pt: 待发送数据指针
  * @param  len: 数据长度
  * @note   线程不安全，若在中断和主循环同时调用需额外保护
  */
void USART1_tx_data(u8 *pt, u8 len)
{
  while(len--)
  {
    // 数据写入发送缓冲区
    txd_scomm1.buffer[txd_scomm1.write_pt] = *pt++;
    txd_scomm1.write_pt = (txd_scomm1.write_pt + 1) % USART1_BUF_SIZE;

    // 临界区保护（防止中断修改cnt）
    NVIC_DisableIRQ(USART1_IRQn);
    txd_scomm1.cnt++;
    NVIC_EnableIRQ(USART1_IRQn);
  }

  // 若发送缓冲区有数据，使能发送中断
  if(txd_scomm1.cnt)
    USART_IntConfig(HT_USART1,  USART_INT_TXC , ENABLE);
}

//-----------------------------------------------------------------------------
/**
  * @brief  USART1测试函数（发送示例数据）
  */
void USART1_test(void)
{
  u8 i, test_array[8];
  for(i=0; i<8; i++)
  {
    test_array[i] = i;  // 填充测试数据（0x00~0x07）
  }
  USART1_tx_data(test_array, 8);  // 发送8字节
}

/**
  * @brief  通过UART1发送单个字节（缓冲区非阻塞式）
  * @param  Data: 要发送的字节
  * @retval 无
  */
 void UART1_SendByte(uint32_t Data)
 {
     // 将数据写入发送缓冲区
     txd_scomm1.buffer[txd_scomm1.write_pt] = (uint8_t)Data;
     txd_scomm1.write_pt = (txd_scomm1.write_pt + 1) % USART1_BUF_SIZE;
 
     // 临界区保护（防止中断修改cnt）
     NVIC_DisableIRQ(USART1_IRQn);
     txd_scomm1.cnt++;
     NVIC_EnableIRQ(USART1_IRQn);
 
     // 触发发送中断
     if (txd_scomm1.cnt)
         USART_IntConfig(HT_USART1, USART_INT_TXC, ENABLE);
 }
 
 /**
   * @brief  通过UART1发送字节数组
   * @param  array: 数组首地址
   * @param  num:   数组长度
   * @retval 无
   */
  void UART1_SendArray(uint8_t *array, uint8_t num)
  {
      USART1_tx_data(array, num); // 直接调用缓冲区发送函数
  }
 
 /**
   * @brief  通过UART1发送字符串（以'\0'结尾）
   * @param  str: 字符串指针
   * @retval 无
   */
  void UART1_SendString(uint8_t *str)
  {
      uint8_t len = 0;
      while (str[len] != '\0') len++; // 计算字符串长度
      USART1_tx_data(str, len);       // 调用缓冲区发送函数
  }
 
 /**
   * 函    数：次方函数（内部使用）
   * 返 回 值：返回值等于X的Y次方
   */
 uint32_t UART1_Pow(uint32_t X, uint32_t Y)
 {
     uint32_t Result = 1;  // 设置结果初值为1
     while (Y--)            // 执行Y次
     {
         Result *= X;      // 将X累乘到结果
     }
     return Result;
 }
 
 /**
   * 函    数：串口发送数字
   * 参    数：Number 要发送的数字，范围：0~4294967295
   * 参    数：Length 要发送数字的长度，范围：0~10
   * 返 回 值：无
   */
  void UART1_SendNumber(uint32_t Number, uint8_t Length)
  {
      uint8_t buffer[10]; // 最大支持10位数字
      uint8_t i;
  
      // 将数字转换为ASCII字符并存入缓冲区
      for (i = 0; i < Length; i++) 
      {
          buffer[i] = (Number / UART1_Pow(10, Length - i - 1) % 10) + '0';
      }
  
      // 通过缓冲区发送
      USART1_tx_data(buffer, Length);
  }
 
  
 /**
   * 函    数：检测UART1接收缓冲区是否有数据
   * 参    数：无
   * 返 回 值：接收缓冲区状态，1表示有数据，0表示空
   * 说    明：非破坏性检查，不修改缓冲区状态
   */
 uint8_t UART1_GetRxStatus(void)
 {
     return (rxd_scomm1.cnt > 0) ? 1 : 0;  // 直接返回缓冲区计数状态
 }
 
 /**
   * 函    数：从UART1接收缓冲区读取一个字节
   * 参    数：无
   * 返 回 值：读取的字节数据（缓冲区为空时返回0xFF）
   * 说    明：破坏性读取，读取后缓冲区指针前移
   */
 uint8_t UART1_ReadRxData(void)
 {
    /* 静态变量提升至函数作用域 -----------------------------------*/
    static uint8_t RxState = 0;    // 状态机状态（整个中断函数可见）
    static uint8_t pRxPacket = 0;  // 数据包位置（整个中断函数可见）

     uint8_t received_data = 0xFF;
     
     NVIC_DisableIRQ(UART1_IRQn);  // 进入临界区，禁用中断
     
     if (rxd_scomm1.cnt > 0)        // 检查缓冲区是否有数据
     {
         received_data = rxd_scomm1.buffer[rxd_scomm1.read_pt];  // 读取数据
         
         /* 更新读指针和计数器 */
         rxd_scomm1.read_pt = (rxd_scomm1.read_pt + 1) % USART1_BUF_SIZE;
         rxd_scomm1.cnt--;
     }
     
     NVIC_EnableIRQ(UART1_IRQn);   // 退出临界区，启用中断
     
     return received_data;
 }

 // UART1中断服务函数
void USART1_IRQHandler(void)
{
  /* 静态变量提升至函数作用域 -----------------------------------*/
  static uint8_t RxState = 0;    // 状态机状态（整个中断函数可见）
  static uint8_t pRxPacket = 0;  // 数据包位置（整个中断函数可见）

  /* 发送中断处理 -----------------------------------*/
  if ((HT_USART1->SR & USART_FLAG_TXC) != RESET)
  {
    USART_ClearFlag(HT_USART1, USART_FLAG_TXC); // 清除发送标志

    // 发送缓冲区管理
    if (txd_scomm1.cnt == 0) // 缓冲区已空
    {
      USART_IntConfig(HT_USART1, USART_INT_TXC, DISABLE);
    }
    else
    {
      uint16_t i;
      // 填充发送FIFO
      for (i = 0; i < USART1_FIFO_LEN; i++)
      {
        USART_SendData(HT_USART1, txd_scomm1.buffer[txd_scomm1.read_pt]);
        txd_scomm1.read_pt = (txd_scomm1.read_pt + 1) % USART1_BUF_SIZE;
        if (--txd_scomm1.cnt == 0)
          break;
      }
    }
  }

  /* 接收中断处理（数据包解析）-----------------------*/
  if ((HT_USART1->SR & USART_FLAG_RXDR) != RESET)
  {
    uint8_t RxData = USART_ReceiveData(HT_USART1);

    // 数据存入环形缓冲区
    rxd_scomm1.buffer[rxd_scomm1.write_pt] = RxData;
    rxd_scomm1.write_pt = (rxd_scomm1.write_pt + 1) % USART1_BUF_SIZE;
    NVIC_DisableIRQ(USART1_IRQn);
    rxd_scomm1.cnt++;
    NVIC_EnableIRQ(USART1_IRQn);
    USART_ClearFlag(HT_USART1, USART_FLAG_RXDR);

    switch (RxState)
    {
    case 0: // 等待包头
      if (RxData == '@' && Serial_RxFlag1 == 0)
      {
//		UART1_SendString("[DEBUG] Received the @\r\n");
        RxState = 1;
        pRxPacket = 0;
        Serial_RxPacket1[pRxPacket++] = RxData; // 存储起始符
      }
      break;

    case 1: // 接收数据
      if (RxData == '.') // 检测包尾
      {
		Serial_RxPacket1[pRxPacket++] = RxData; // 保存"."到数组
        Serial_RxPacket1[pRxPacket] = '\0'; // 字符串终止
//		UART1_SendString("[DEBUG] Received .\r\n");
//        UART1_SendString("[DEBUG] Packet End: ");
        UART1_SendString(Serial_RxPacket1); // 显示完整包
        UART1_SendString("\r\n");
        Serial_RxFlag1 = 1;  // 设置接收完成标志
        RxState = 0;         // 重置状态机
      }
      else
      {
        if (pRxPacket < (SERIAL_RX_PACKET_SIZE - 1))
        {
          Serial_RxPacket1[pRxPacket++] = RxData;
        }
        else
        {
//		UART1_SendString("[ERROR] Buffer Overflow!\r\n");
          // 缓冲区溢出处理
          RxState = 0;
          pRxPacket = 0;
          Serial_RxFlag1 = 0xFF; // 错误标志
        }
      }
      break;
    }
  }

//  /* 接收超时处理（清除残留数据）---------------------*/
//	if ((HT_USART1->SR & USART_FLAG_TOUT) != RESET) 
//{
//	USART_ClearFlag(HT_USART1, USART_FLAG_TOUT);
//	// 清理残留数据
//	while (USART_GetFIFOStatus(HT_USART1, USART_FIFO_RX)) 
//	{
//    (void)USART_ReceiveData(HT_USART1);
//	}
//  // 重置状态机
//	RxState = 0;
//	pRxPacket = 0;
//  // 标记超时错误（仅当无其他错误时）
//  if (Serial_RxFlag1 == 0) Serial_RxFlag1 = 0xFE; 
//}
}

// 发送数据包
void SendPacket_USART1(const char* data)
{
  // 添加包头
  txd_scomm1.buffer[txd_scomm1.write_pt++] = '@';
  txd_scomm1.cnt++;
  
  // 填充数据
  while(*data && txd_scomm1.cnt < USART1_BUF_SIZE-2) // 保留包尾和空位
  {
    txd_scomm1.buffer[txd_scomm1.write_pt++] = *data++;
    txd_scomm1.cnt++;
  }
  
  // 添加包尾
  txd_scomm1.buffer[txd_scomm1.write_pt++] = '.';
  txd_scomm1.cnt++;
  
  // 使能发送中断
  USART_IntConfig(HT_USART1, USART_INT_TXC, ENABLE);
}


