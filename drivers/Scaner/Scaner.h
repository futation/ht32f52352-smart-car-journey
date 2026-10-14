#ifndef __SCANER_H
#define __SCANER_H

//-----------------------------------------------------------------------------
#include "ht32.h"

//-----------------------------------------------------------------------------
#define USART1_BUF_SIZE 256
#define USART1_FIFO_LEN 1

//-----------------------------------------------------------------------------
typedef struct
{
  u8 buffer[USART1_BUF_SIZE];
  u16 write_pt;
  u16 read_pt;
  u16 cnt;
}_USART1_STRUCT;

#define SERIAL_RX_PACKET_SIZE  128
extern uint8_t Serial_RxPacket1[SERIAL_RX_PACKET_SIZE];
extern volatile uint8_t Serial_RxFlag1;

//-----------------------------------------------------------------------------
void USART1_Configuration(void);
void USART1_init_buffer (void);
void USART1_analyze_data(void);
void USART1_tx_data(u8 *pt, u8 len);
void USART1_test(void);
void UART1_SendByte(uint32_t Data);
void UART1_SendArray(uint8_t *array, uint8_t num);
void UART1_SendString(uint8_t *str);
uint32_t UART1_Pow(uint32_t X, uint32_t Y);
void UART1_SendNumber(uint32_t Number, uint8_t Length);
uint8_t UART1_GetRxStatus(void);
uint8_t UART1_ReadRxData(void);

//-----------------------------------------------------------------------------
#endif
