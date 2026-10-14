#ifndef __UART0_H
#define __UART0_H


//-----------------------------------------------------------------------------
#include "ht32.h"

//-----------------------------------------------------------------------------
#define UART0_BUF_SIZE 256
#define UART0_FIFO_LEN 1

//-----------------------------------------------------------------------------
typedef struct
{
  u8 buffer[UART0_BUF_SIZE];
  u16 write_pt;
  u16 read_pt;
  u16 cnt;
}_UART0_STRUCT;

extern uint8_t Serial_RxPacket[];
extern volatile uint8_t Serial_RxFlag;

//-----------------------------------------------------------------------------
void UART0_Configuration(void);
void UART0_init_buffer (void);
void UART0_analyze_data(void);
void UART0_tx_data(u8 *pt, u8 len);
void UART0_test(void);
void UART0_SendByte(uint32_t Data);
void UART0_SendArray(uint8_t *array, uint8_t num);
void UART0_SendString(uint8_t *str);
uint32_t UART0_Pow(uint32_t X, uint32_t Y);
void UART0_SendNumber(uint32_t Number, uint8_t Length);
uint8_t UART0_GetRxStatus(void);
uint8_t UART0_ReadRxData(void);
//-----------------------------------------------------------------------------
#endif

