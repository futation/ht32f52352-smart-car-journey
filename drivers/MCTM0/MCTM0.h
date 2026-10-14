/* 文档2：MCTM0.h */
#ifndef __MCTM0_H
#define __MCTM0_H

#include "ht32.h"

/* 函数声明 */
void MCTM0_Configuration(void);
void MCTM0_SetFrequency(u32 freq);
void MCTM0_CH2_SetOnduty(u16 percent);
void MCTM0_CH2_EnablePWMOutput(void);
void MCTM0_CH2_DisablePWMOutput(u8 IO_State);
void MCTM0_CH3_SetOnduty(u16 percent);
void MCTM0_CH3_EnablePWMOutput(void);
void MCTM0_CH3_DisablePWMOutput(u8 IO_State);

#endif

