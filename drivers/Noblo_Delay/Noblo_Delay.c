// File: delay.c

#include "Noblo_Delay.h"
#include "UART0.h"
#include "BFTM0.h"

extern vu32 bftm0_ct;

void Delay_Init(void)
{
    BFTM0_Configuration();  // 使用原厂提供的定时器配置
}

 
uint8_t Delay_NonBlocking(DelayTimer *timer, vu32 delay_ms)         
{
	// 调试输出当前计时器状态
    static uint32_t last_print = 0;
    if (bftm0_ct - last_print > 1000000) // 每秒打印一次
    {
//      UART0_SendString("[Delay] Start: ");
//      UART0_SendNumber (timer->start_time,5);
//		UART0_SendString(" Current: ");
//		UART0_SendNumber (bftm0_ct,5);
//		UART0_SendString(" Target: ");
//		UART0_SendNumber (timer->delay_ms,5);
//		UART0_SendString("\r\n");
                       
        last_print = bftm0_ct;
    }
    // 延时未启动，则初始化新延时
    if(!timer->is_running)  
    {
        timer->start_time = bftm0_ct; // 记录开始时间
        timer->delay_ms = delay_ms;  // 设置延时时间
        timer->is_running = 1;  // 标记为运行中
        return 0;  // 延时成功启动
    }
    
    // 处理32位计数器溢出，计算已过时间
    vu32 elapsed = bftm0_ct - timer->start_time;
    
    // 如果已过时间大于或等于设定的延时时间
    if(elapsed >= timer->delay_ms) 
    {
        timer->is_running = 0;
        return 1;
    }
    return 0;
}
