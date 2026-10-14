//-----------------------------------------------------------------------------
#include "UART0.h"

//-----------------------------------------------------------------------------
// 定义UART0的接收和发送缓冲区结构体，并使用4字节对齐
__ALIGN4 _UART0_STRUCT rxd_comm0; // 接收缓冲区
__ALIGN4 _UART0_STRUCT txd_comm0; // 发送缓冲区

// 添加以下全局变量定义（放在合适的位置，如头文件或全局区域）
#define SERIAL_RX_PACKET_SIZE 64                         // 根据需求调整数据包最大长度
__ALIGN4 uint8_t Serial_RxPacket[SERIAL_RX_PACKET_SIZE]; // 保证内存对齐
uint8_t Serial_RxPacket[SERIAL_RX_PACKET_SIZE];
volatile uint8_t Serial_RxFlag = 0;

//-----------------------------------------------------------------------------
// UART0初始化配置函数
void UART0_Configuration(void)
{
  USART_InitTypeDef USART_InitStruct;              // USART初始化结构体
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}}; // 外设时钟配置结构体

  // 使能UART0、AFIO和GPIOC外设的时钟
  CKCUClock.Bit.UART0 = 1; // 启用UART0时钟
  CKCUClock.Bit.AFIO = 1;  // 启用AFIO时钟（用于引脚复用功能）
  CKCUClock.Bit.PC = 1;    // 启用GPIOC时钟
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  // 配置GPIOC的PIN5（UART0_RX）上拉电阻，防止未知状态
  GPIO_PullResistorConfig(HT_GPIOC, GPIO_PIN_5, GPIO_PR_UP);

  // 配置GPIOC的PIN4和PIN5为UART0的复用功能
  AFIO_GPxConfig(GPIO_PC, AFIO_PIN_4, AFIO_FUN_USART_UART); // 配置PC4为UART0_TX
  AFIO_GPxConfig(GPIO_PC, AFIO_PIN_5, AFIO_FUN_USART_UART); // 配置PC5为UART0_RX

  // 配置UART0的参数
  USART_InitStruct.USART_BaudRate = 9600;                  // 波特率：9600
  USART_InitStruct.USART_WordLength = USART_WORDLENGTH_8B; // 数据位：8位
  USART_InitStruct.USART_StopBits = USART_STOPBITS_1;      // 停止位：1位
  USART_InitStruct.USART_Parity = USART_PARITY_NO;         // 校验位：无
  USART_InitStruct.USART_Mode = USART_MODE_NORMAL;         // 模式：普通模式
  USART_Init(HT_UART0, &USART_InitStruct);                 // 初始化UART0

  // 使能UART0的接收和发送功能
  USART_RxCmd(HT_UART0, ENABLE); // 使能接收
  USART_TxCmd(HT_UART0, ENABLE); // 使能发送

  // 使能UART0的接收中断
  USART_IntConfig(HT_UART0, USART_INT_RXDR, ENABLE);

  // 使能UART0的中断
  NVIC_EnableIRQ(UART0_IRQn);

  // 初始化UART0的接收和发送缓冲区
  UART0_init_buffer();
}

//-----------------------------------------------------------------------------
// 初始化UART0的接收和发送缓冲区
void UART0_init_buffer(void)
{
  // 初始化接收缓冲区
  rxd_comm0.read_pt = 0;  // 读指针
  rxd_comm0.write_pt = 0; // 写指针
  rxd_comm0.cnt = 0;      // 数据计数

  // 初始化发送缓冲区
  txd_comm0.read_pt = 0;  // 读指针
  txd_comm0.write_pt = 0; // 写指针
  txd_comm0.cnt = 0;      // 数据计数
}

//-----------------------------------------------------------------------------
// UART0数据解析函数
void UART0_analyze_data(void)
{
  u8 tmp;
  if (!rxd_comm0.cnt) // 如果接收缓冲区为空，直接返回
    return;

  // 示例：检查接收数据的有效性
  tmp = rxd_comm0.buffer[rxd_comm0.read_pt]; // 读取接收缓冲区的数据
  if ((tmp != 0xAA))                         // 如果数据无效（示例：不等于0xAA）
  {
    NVIC_DisableIRQ(UART0_IRQn);                                  // 禁用UART0中断
    rxd_comm0.cnt--;                                              // 丢弃无效数据
    NVIC_EnableIRQ(UART0_IRQn);                                   // 重新使能UART0中断
    rxd_comm0.read_pt = (rxd_comm0.read_pt + 1) % UART0_BUF_SIZE; // 更新读指针
    return;
  }
  else if (rxd_comm0.cnt >= 8) // 如果接收缓冲区有足够的数据（示例：8字节）
  {
    // 在这里添加数据解析逻辑
  }
}

//-----------------------------------------------------------------------------
// UART0发送数据函数
void UART0_tx_data(u8 *pt, u8 len)
{
  while (len--) // 遍历待发送的数据
  {
    txd_comm0.buffer[txd_comm0.write_pt] = *pt++;                   // 将数据存入发送缓冲区
    txd_comm0.write_pt = (txd_comm0.write_pt + 1) % UART0_BUF_SIZE; // 更新写指针
    NVIC_DisableIRQ(UART0_IRQn);                                    // 禁用UART0中断
    txd_comm0.cnt++;                                                // 更新数据计数
    NVIC_EnableIRQ(UART0_IRQn);                                     // 重新使能UART0中断
  }

  if (txd_comm0.cnt)                                  // 如果发送缓冲区有数据
    USART_IntConfig(HT_UART0, USART_INT_TXC, ENABLE); // 使能发送中断
}

//-----------------------------------------------------------------------------
// UART0测试函数
void UART0_test(void)
{
  u8 i, test_array[8];
  for (i = 0; i < 8; i++) // 初始化测试数据
  {
    test_array[i] = i;
  }
  UART0_tx_data(test_array, 8); // 发送测试数据
}

//-----------------------------------------------------------------------------

/**
 * @brief  通过UART0发送单个字节
 * @param  Data: 要发送的字节
 * @retval 无
 */
// 弃用阻塞式发送，全部改用缓冲队列
void UART0_SendByte(uint32_t Data)
{
  // 将数据写入发送缓冲区
  txd_comm0.buffer[txd_comm0.write_pt] = (uint8_t)Data;
  txd_comm0.write_pt = (txd_comm0.write_pt + 1) % UART0_BUF_SIZE;

  // 临界区保护（防止中断修改cnt）
  NVIC_DisableIRQ(UART0_IRQn);
  txd_comm0.cnt++;
  NVIC_EnableIRQ(UART0_IRQn);

  // 触发发送中断
  if (txd_comm0.cnt)
    USART_IntConfig(HT_UART0, USART_INT_TXC, ENABLE);
}

/**
 * @brief  通过UART0发送字节数组
 * @param  array: 数组首地址
 * @param  num:   数组长度
 * @retval 无
 */
void UART0_SendArray(uint8_t *array, uint8_t num)
{
  UART0_tx_data(array, num); // 直接调用缓冲区发送函数
}

/**
 * @brief  通过UART0发送字符串（以'\0'结尾）
 * @param  str: 字符串指针
 * @retval 无
 */
void UART0_SendString(uint8_t *str)
{
  uint8_t len = 0;
  while (str[len] != '\0')
    len++;                 // 计算字符串长度
  UART0_tx_data(str, len); // 调用缓冲区发送函数
}

/**
 * 函    数：次方函数（内部使用）
 * 返 回 值：返回值等于X的Y次方
 */
uint32_t UART0_Pow(uint32_t X, uint32_t Y)
{
  uint32_t Result = 1; // 设置结果初值为1
  while (Y--)          // 执行Y次
  {
    Result *= X; // 将X累乘到结果
  }
  return Result;
}

/**
 * 函    数：串口发送数字
 * 参    数：Number 要发送的数字，范围：0~4294967295
 * 参    数：Length 要发送数字的长度，范围：0~10
 * 返 回 值：无
 */
void UART0_SendNumber(uint32_t Number, uint8_t Length)
{
  uint8_t i;
  for (i = 0; i < Length; i++) // 根据数字长度遍历数字的每一位
  {
    UART0_SendByte(Number / UART0_Pow(10, Length - i - 1) % 10 + '0');
  }
}

/**
 * 函    数：检测UART0接收缓冲区是否有数据
 * 参    数：无
 * 返 回 值：接收缓冲区状态，1表示有数据，0表示空
 * 说    明：非破坏性检查，不修改缓冲区状态
 */
uint8_t UART0_GetRxStatus(void)
{
  return (rxd_comm0.cnt > 0) ? 1 : 0; // 直接返回缓冲区计数状态
}

/**
 * 函    数：从UART0接收缓冲区读取一个字节
 * 参    数：无
 * 返 回 值：读取的字节数据（缓冲区为空时返回0xFF）
 * 说    明：破坏性读取，读取后缓冲区指针前移
 */
uint8_t UART0_ReadRxData(void)
{
  uint8_t received_data = 0xFF;

  NVIC_DisableIRQ(UART0_IRQn); // 进入临界区，禁用中断

  if (rxd_comm0.cnt > 0) // 检查缓冲区是否有数据
  {
    received_data = rxd_comm0.buffer[rxd_comm0.read_pt]; // 读取数据

    /* 更新读指针和计数器 */
    rxd_comm0.read_pt = (rxd_comm0.read_pt + 1) % UART0_BUF_SIZE;
    rxd_comm0.cnt--;
  }

  NVIC_EnableIRQ(UART0_IRQn); // 退出临界区，启用中断

  return received_data;
}

// UART0中断服务函数
void UART0_IRQHandler(void)
{
  // 发送中断处理
  if ((HT_UART0->SR & USART_FLAG_TXC) != RESET)
  {
    USART_ClearFlag(HT_UART0, USART_FLAG_TXC); // 清除发送完成标志

    if (txd_comm0.cnt == 0) // 发送缓冲区空
    {
      USART_IntConfig(HT_UART0, USART_INT_TXC, DISABLE); // 关闭发送中断
    }
    else
    {
      uint16_t i;
      // 填充UART发送FIFO
      for (i = 0; i < UART0_FIFO_LEN; i++)
      {
        USART_SendData(HT_UART0, txd_comm0.buffer[txd_comm0.read_pt]);
        txd_comm0.read_pt = (txd_comm0.read_pt + 1) % UART0_BUF_SIZE;
        if (--txd_comm0.cnt == 0)
          break; // 缓冲区已空退出
      }
    }
  }

  // 接收中断处理（数据包解析）
  if ((HT_UART0->SR & USART_FLAG_RXDR) != RESET)
  {
    static uint8_t RxState = 0;   // 状态机状态
    static uint8_t pRxPacket = 0; // 数据包位置

    uint8_t RxData = USART_ReceiveData(HT_UART0);

    USART_ClearFlag(HT_UART0, USART_FLAG_RXDR); // 清除接收标志

    switch (RxState)
    {
    case 0: // 等待包头
      if (RxData == '@' && Serial_RxFlag == 0)
      {
        // UART0_SendString("[DEBUG] Received the @\r\n");
        RxState = 1;
        pRxPacket = 0;
        Serial_RxPacket[pRxPacket++] = RxData; // 存储起始字符
      }
      break;

    case 1:              // 接收数据
      if (RxData == '.') // 检测包尾
      {
        Serial_RxPacket[pRxPacket++] = RxData; // 保存"."到数组
        Serial_RxPacket[pRxPacket] = '\0'; // 终止字符串
        // UART0_SendString("[DEBUG] Received .\r\n");
        // UART0_SendString("[DEBUG] Packet End: ");
        //UART0_SendString(Serial_RxPacket); // 显示完整包
        //UART0_SendString("\r\n");
        Serial_RxFlag = 1;
        RxState = 0; // 重置状态机
      }
      else
      {
        if (pRxPacket < (SERIAL_RX_PACKET_SIZE - 1)) // 保留1字节给.
        {
          Serial_RxPacket[pRxPacket++] = RxData;
          // 调试信息（可选）
          //UART0_SendByte(RxData); // 字符回显
        }
        else
        {
          // UART0_SendString("[ERROR] Buffer Overflow!\r\n");
          RxState = 0;
          pRxPacket = 0;
        }
      }
      break;
    }
  }
}
