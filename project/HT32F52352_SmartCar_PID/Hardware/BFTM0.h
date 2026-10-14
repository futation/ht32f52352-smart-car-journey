
#ifndef __BFTM0_H
#define __BFTM0_H


//-----------------------------------------------------------------------------
#include "ht32.h"

extern vu32 bftm0_ct;
void BFTM0_Configuration(void);
#define Get_SystemTick()  (bftm0_ct)  // 直接返回BFTM0的计数值

#endif

