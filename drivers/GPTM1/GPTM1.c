#include "GPTM1.h"

//-----------------------------------------------------------------------------
void GPTM1_Configuration(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  
  // 使能外设时钟
  CKCUClock.Bit.AFIO  = 1;
  CKCUClock.Bit.GPTM1 = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  // 配置编码器输入引脚
  AFIO_GPxConfig(GPIO_PA, AFIO_PIN_0, AFIO_FUN_MCTM_GPTM); // 通道0（A相）
  AFIO_GPxConfig(GPIO_PA, AFIO_PIN_1, AFIO_FUN_MCTM_GPTM); // 通道1（B相）

  // 配置正交解码器模式
  TM_DecoderConfig(HT_GPTM1, 
                  TM_SMSEL_DECODER3,          // 解码器模式3（A/B相正交编码）
                  TM_CHP_NONINVERTED,            // 通道0极性（根据实际接线调整）
                  TM_CHP_NONINVERTED);        // 通道1极性

  // 使能定时器
  TM_Cmd(HT_GPTM1, ENABLE);
}

//-----------------------------------------------------------------------------
u32 Encoder1_Get(void)
{
  return TM_GetCounter(HT_GPTM1); // 直接读取32位计数器值
}


void Encoder1_Reset(void)
{
    TM_SetCounter(HT_GPTM1, 0); 
}
