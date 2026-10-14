#include "GPTM0.h"

//-----------------------------------------------------------------------------
static vu32 gptm0_cntr_cnt;  // 用于溢出计数（可选）

//-----------------------------------------------------------------------------
void GPTM0_Configuration(void)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
  CKCUClock.Bit.AFIO  = 1;     // 启用 AFIO 时钟
  CKCUClock.Bit.GPTM0 = 1;     // 启用 GPTM0 时钟
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  // 配置 PA4（CH0）和 PA5（CH1）为编码器输入引脚
  AFIO_GPxConfig(GPIO_PA, AFIO_PIN_4, AFIO_FUN_MCTM_GPTM);  // A相（CH0）
  AFIO_GPxConfig(GPIO_PA, AFIO_PIN_5, AFIO_FUN_MCTM_GPTM);  // B相（CH1）

  // 配置正交编码器模式（Decoder Mode 3）
  TM_DecoderConfig(HT_GPTM0, 
                   TM_SMSEL_DECODER3,          // 正交解码模式
                   TM_CHP_NONINVERTED,         // CH0 极性不反转
                   TM_CHP_NONINVERTED);        // CH1 极性不反转

  // 启用定时器计数
  TM_Cmd(HT_GPTM0, ENABLE);
}

//-----------------------------------------------------------------------------
/**
  * @brief  读取编码器计数值
  * @retval 当前编码器计数值（32位）
  */
u32 Encoder_Get(void)
{
  return TM_GetCounter(HT_GPTM0);
}

/**
  * @brief  重置编码器计数值
  * @retval 无
  */
void Encoder_Reset(void)
{
    TM_SetCounter(HT_GPTM0, 0);
}
