#include "BFTM0.h"  // 包含BFTM0模块的头文件，定义相关寄存器地址和函数原型

//-----------------------------------------------------------------------------
// 定时器基准值计算：系统时钟频率/1000，用于生成1ms间隔的中断
// 假设SystemCoreClock=48MHz时，BFTM0_TIMER_BASE=48000 → 48MHz/48000=1000Hz
#define BFTM0_TIMER_BASE  ((long long)SystemCoreClock * 1/1000)

//-----------------------------------------------------------------------------
volatile uint32_t bftm0_ct;  // 定义易变类型全局变量，用于中断计数
                             // volatile关键字确保变量在中断和主程序中的可见性

//-----------------------------------------------------------------------------
// BFTM0定时器配置函数
// 功能：初始化基本定时器0，设置时钟、中断和计数器参数
//-----------------------------------------------------------------------------
void BFTM0_Configuration(void)
{
  // 时钟控制单元配置结构体初始化
  CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
  CKCUClock.Bit.BFTM0 = 1;        // 开启BFTM0外设时钟（参考网页8类似配置）
  CKCU_PeripClockConfig(CKCUClock, ENABLE);  // 应用时钟配置

  NVIC_EnableIRQ(BFTM0_IRQn);     // 使能BFTM0中断通道（嵌套向量中断控制器配置）
  BFTM_IntConfig(HT_BFTM0, ENABLE); // 使能定时器中断源

  // 定时器参数设置（参考网页7的BFTM配置逻辑）
  BFTM_SetCompare(HT_BFTM0, BFTM0_TIMER_BASE); // 设置比较寄存器值，决定中断间隔
  BFTM_SetCounter(HT_BFTM0, 0);    // 清零计数器初始值
  BFTM_EnaCmd(HT_BFTM0, ENABLE);   // 使能定时器运行
}

//-----------------------------------------------------------------------------
// BFTM0中断服务函数
// 功能：处理定时器溢出中断，维护时间计数并执行附加检测
//-----------------------------------------------------------------------------
void BFTM0_IRQHandler(void)
{
  BFTM_ClearFlag(HT_BFTM0);    // 清除中断标志位（必须操作，否则会重复进入中断）
  bftm0_ct++;                   // 全局计数器递增，用于时间基准计算
}

