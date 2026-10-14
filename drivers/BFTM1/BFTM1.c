#include "BFTM1.h"

//-----------------------------------------------------------------------------
#define BFTM1_TIMER_BASE            ((long long)SystemCoreClock * 1/1000)

//-----------------------------------------------------------------------------
vu32 bftm1_ct;

//-----------------------------------------------------------------------------
void BFTM1_Configuration(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.BFTM1      = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  NVIC_EnableIRQ(BFTM1_IRQn);   // interrupt enable
  BFTM_IntConfig(HT_BFTM1, ENABLE);

  BFTM_SetCompare(HT_BFTM1, BFTM1_TIMER_BASE);
  BFTM_SetCounter(HT_BFTM1, 0);
  BFTM_EnaCmd(HT_BFTM1, ENABLE);
}

// //-----------------------------------------------------------------------------
// void BFTM1_IRQHandler(void)
// {
//   BFTM_ClearFlag(HT_BFTM1);
//   bftm1_ct++;
// }

// //-----------------------------------------------------------------------------


