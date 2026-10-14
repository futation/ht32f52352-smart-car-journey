#include "ht32f5xxxx_01.h"              // Device header

/**
  * @brief  微秒级延时
  * @param  xus 延时时长，范围：0~233015
  * @retval 无
  */
 void Delay_us(uint32_t xus) 
 {
	 uint32_t cycles_needed = 48 * xus; // 48 cycles/μs
	 
	 // 检查是否超过 24 位计数器最大值（0xFFFFFF）
	 if (cycles_needed > 0xFFFFFF) 
		 {
		 // 错误处理：可返回或限制最大值（约 349ms）
		 return;
		 }
	 
	 SysTick->LOAD = cycles_needed - 1;  // 必须减1（24位递减计数器）
	 SysTick->VAL  = 0x00;               // 清空当前值
	 SysTick->CTRL = 0x05;               // 启动定时器（HCLK，不使能中断）
	 
	 while (!(SysTick->CTRL & 0x00010000)); // 等待计数到0（检查 COUNTFLAG）
	 
	 SysTick->CTRL = 0x04;               // 关闭定时器
 }

/**
  * @brief  毫秒级延时
  * @param  xms 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
	while(xms--)
	{
		Delay_us(1000);
	}
}
 
/**
  * @brief  秒级延时
  * @param  xs 延时时长，范围：0~4294967295
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
	while(xs--)
	{
		Delay_ms(1000);
	}
} 
