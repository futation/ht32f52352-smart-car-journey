/* 文档1：MCTM0.c */
//-----------------------------------------------------------------------------
#include "MCTM0.h"
//-----------------------------------------------------------------------------
/* 宏定义 */
#define MCTM0_TIMER_BASE            ((long long)SystemCoreClock / 1000)  // 基础时钟分频为1KHz


//-----------------------------------------------------------------------------
/**​
  * @brief  MCTM0定时器及PWM通道配置
  * @note   配置MCTM0为PWM模式，初始化通道2和3用于电机控制
  */
void MCTM0_Configuration(void)
{
  u32 wCRR = 0, wPSCR = 0;
  TM_TimeBaseInitTypeDef TM_TimeBaseInitStruct;
  TM_OutputInitTypeDef TM_OutputInitStruct;
  
  /* 时钟配置：使能MCTM0、AFIO时钟 */
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.MCTM0      = 1;
  CKCUClock.Bit.AFIO       = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  /* GPIO复用功能配置：仅配置电机相关引脚 */
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_6, AFIO_FUN_MCTM_GPTM);  // PB6复用为MCTM0_CH2
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_8, AFIO_FUN_MCTM_GPTM);  // PB8复用为MCTM0_CH3

  /* 计算定时器预分频和重装载值 */
  wCRR = MCTM0_TIMER_BASE - 1;  // 计算目标频率下的计数器值
  while ((wCRR / (wPSCR + 1)) > 0xFFFF)  // 确保计数值不超过16位
  {
    wPSCR++;
  }
  wCRR = wCRR / (wPSCR + 1);     // 最终重装载值

 /* 定时器基础配置 */
 TM_TimeBaseInitStruct.CounterReload = wCRR;          // 自动重装载值
 TM_TimeBaseInitStruct.Prescaler = wPSCR;             // 预分频系数
 TM_TimeBaseInitStruct.RepetitionCounter = 0;         // 重复计数器（高级定时器用）
 TM_TimeBaseInitStruct.CounterMode = TM_CNT_MODE_UP; // 向上计数模式
 TM_TimeBaseInitStruct.PSCReloadTime = TM_PSC_RLD_IMMEDIATE; // 预分频立即生效
 TM_TimeBaseInit(HT_MCTM0, &TM_TimeBaseInitStruct);

  /* 通道2配置：电机控制PWM输出 */
  TM_OutputInitStruct.Channel = TM_CH_2;
  TM_OutputInitStruct.OutputMode = TM_OM_PWM2;        // PWM模式2（CNT < CCR时有效）
  TM_OutputInitStruct.Control = TM_CHCTL_ENABLE;      // 主输出使能
  TM_OutputInitStruct.ControlN = TM_CHCTL_ENABLE;     // 互补输出使能
  TM_OutputInitStruct.Polarity = TM_CHP_NONINVERTED;  // 输出极性不反转
  TM_OutputInitStruct.PolarityN = TM_CHP_NONINVERTED; // 互补输出极性
  TM_OutputInitStruct.IdleState = MCTM_OIS_LOW;       // 空闲状态低电平
  TM_OutputInitStruct.IdleStateN = MCTM_OIS_HIGH;     // 互补通道空闲高电平
  TM_OutputInitStruct.Compare = ((wCRR + 1) * (100 - 50)) / 100; // 初始占空比50%
  TM_OutputInit(HT_MCTM0, &TM_OutputInitStruct);

  TM_OutputInitStruct.Channel = TM_CH_3;  // 通道3配置
  TM_OutputInitStruct.OutputMode = TM_OM_PWM2;  // PWM模式2
  TM_OutputInitStruct.Control = TM_CHCTL_ENABLE;  // 主输出使能
  TM_OutputInitStruct.ControlN = TM_CHCTL_DISABLE;  // 互补输出禁止
  TM_OutputInitStruct.Polarity = TM_CHP_NONINVERTED;  // 输出极性不反转
  TM_OutputInitStruct.PolarityN = TM_CHP_NONINVERTED;  // 互补输出极性
  TM_OutputInitStruct.IdleState = MCTM_OIS_LOW;  // 空闲状态低电平
  TM_OutputInitStruct.IdleStateN = MCTM_OIS_HIGH;  // 互补通道空闲高电平
  TM_OutputInitStruct.Compare = ((wCRR + 1) * (100 - 50)) / 100;  // 初始占空比50%
  TM_OutputInit(HT_MCTM0, &TM_OutputInitStruct);

  /* 使能主输出并启动定时器 */
  MCTM_CHMOECmd(HT_MCTM0, ENABLE);  // 主输出使能
  TM_Cmd(HT_MCTM0, ENABLE);         // 启动定时器
}

//-----------------------------------------------------------------------------
/**​
  * @brief  设置PWM频率
  * @param  freq 目标频率（单位：Hz）
  */
void MCTM0_SetFrequency(u32 freq)
{
  TM_TimeBaseInitTypeDef TM_TimeBaseInitStruct;
  uint32_t wCRR = 0, wPSCR = 0;

  /* 重新计算预分频和重装载值 */
  wCRR = (SystemCoreClock / freq) - 1;
  while ((wCRR / (wPSCR + 1)) > 0xFFFF)
  {
    wPSCR++;
  }
  wCRR = wCRR / (wPSCR + 1);

  /* 更新定时器配置 */
  TM_TimeBaseInitStruct.CounterReload = wCRR;
  TM_TimeBaseInitStruct.Prescaler = wPSCR;
  TM_TimeBaseInitStruct.RepetitionCounter = 0;
  TM_TimeBaseInitStruct.CounterMode = TM_CNT_MODE_UP;
  TM_TimeBaseInitStruct.PSCReloadTime = TM_PSC_RLD_IMMEDIATE;
  TM_TimeBaseInit(HT_MCTM0, &TM_TimeBaseInitStruct);

  /* 重置占空比为0（安全操作） */
  TM_SetCaptureCompare2(HT_MCTM0, 0); // CH2
  TM_SetCaptureCompare3(HT_MCTM0, 0); // CH3
}


//------------------------------ 通道2/3控制函数 -----------------------------
/** 
  * @brief  设置通道2占空比
  * @param  percent 占空比（0-100）
  */
void MCTM0_CH2_SetOnduty(u16 percent)
{
  u16 cmp;
  percent = (percent > 100) ? 100 : percent;        // 限幅
  cmp = HT_MCTM0->CRR * (100 - percent) / 100;      // 计算比较值
  TM_SetCaptureCompare2(HT_MCTM0, cmp);             // 更新CCR2
}

/**​
  * @brief  使能通道2 PWM输出
  */
void MCTM0_CH2_EnablePWMOutput(void)
{
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_6, AFIO_FUN_MCTM_GPTM);  // 重新启用复用功能
}

/**​
  * @brief  禁用通道2 PWM输出并设置GPIO状态
  * @param  IO_State 输出电平（0:低 1:高）
  */
void MCTM0_CH2_DisablePWMOutput(u8 IO_State)
{
  /* 使能GPIO时钟 */
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.AFIO = 1;
  CKCUClock.Bit.PB = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  /* 设置GPIO为固定电平 */
  if(IO_State)
    GPIO_SetOutBits  (HT_GPIOB, AFIO_PIN_6);
  else
    GPIO_ClearOutBits(HT_GPIOB, AFIO_PIN_6);
  GPIO_DirectionConfig(HT_GPIOB, AFIO_PIN_6, GPIO_DIR_OUT);
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_6, AFIO_FUN_GPIO);  // 恢复为普通IO
}


/* 通道3的同类型函数（与通道2类似，省略部分注释） */
void MCTM0_CH3_SetOnduty(u16 percent)
{
  u16 cmp;
  if(percent > 100)
    percent = 100;
  cmp = HT_MCTM0->CRR * (100-percent) / 100;
  TM_SetCaptureCompare3(HT_MCTM0, cmp);
}


void MCTM0_CH3_EnablePWMOutput(void)
{
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_8, AFIO_FUN_MCTM_GPTM);
}

void MCTM0_CH3_DisablePWMOutput(u8 IO_State)
{
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.AFIO = CKCUClock.Bit.PB = 1;
  CKCU_PeripClockConfig(CKCUClock, ENABLE);

  if(IO_State)
    GPIO_SetOutBits  (HT_GPIOB, AFIO_PIN_8);  
  else
    GPIO_ClearOutBits(HT_GPIOB, AFIO_PIN_8);
  GPIO_DirectionConfig(HT_GPIOB, AFIO_PIN_8, GPIO_DIR_OUT);
  AFIO_GPxConfig(GPIO_PB, AFIO_PIN_8, AFIO_FUN_GPIO);
}



