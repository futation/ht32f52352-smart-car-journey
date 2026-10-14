//-----------------------------------------------------------------------------
#include "LED.h"

//-----------------------------------------------------------------------------
void LED_Configuration(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.PC    = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);



  GPIO_ClearOutBits       (OUT_PC14_GPIO_PORT, OUT_PC14_GPIO_PIN);
  GPIO_DirectionConfig    (OUT_PC14_GPIO_PORT, OUT_PC14_GPIO_PIN, GPIO_DIR_OUT);
}

//-----------------------------------------------------------------------------
