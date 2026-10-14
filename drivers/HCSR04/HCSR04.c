/* HCSR04.c - 超声波测距模块HC-SR04驱动代码（使用SCTM0定时器）*/

#include "HCSR04.h"     
#include "Delay.h"      
#include <string.h>     

/*********************** 硬件配置宏定义 **************************/
#define TRIG_PORT    HT_GPIOB      // 触发信号引脚所在端口
#define TRIG_PIN     GPIO_PIN_0    // 触发信号引脚PB0
#define ECHO_PORT    HT_GPIOB      // 回波信号引脚所在端口
#define ECHO_PIN     GPIO_PIN_1    // 回波信号引脚PB1

/*********************** 定时器配置参数 **************************/
#define TIMER_BASE_CLK       (SystemCoreClock)  // 定时器基准时钟
#define SAMPLE_COUNT         5                  // 采样次数
#define MEASURE_TIMEOUT      100000             // 测量超时阈值
#define SOUND_SPEED          343.0f             // 声速（m/s）

/***************************************************************
* @函数名    HCSR04_Init
* @描述      超声波模块硬件初始化
* @参数      无
* @返回值    无
* @备注      1.配置Trig和Echo引脚
*           2.初始化SCTM0定时器用于脉冲宽度测量
*           3.通过串口输出定时器配置参数用于验证
***************************************************************/
void HCSR04_Init(void)
{
	
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    TM_TimeBaseInitTypeDef TIM_InitStruct;
    
    /*-------- GPIO配置 --------*/
    // 启用GPIOB时钟
    CKCUClock.Bit.PB = 1;
    // CKCUClock.Bit.SCTM0 = 1; // 使能SCTM0定时器时钟
    CKCU_PeripClockConfig(CKCUClock, ENABLE);
    
    // 配置Trig引脚
    GPIO_ClearOutBits(TRIG_PORT, TRIG_PIN);
    GPIO_DirectionConfig(TRIG_PORT, TRIG_PIN, GPIO_DIR_OUT);
    GPIO_WriteOutBits(TRIG_PORT,TRIG_PIN,RESET);
    
    // 配置Echo引脚
    GPIO_PullResistorConfig(ECHO_PORT, ECHO_PIN, GPIO_PR_DOWN);
    GPIO_DirectionConfig(ECHO_PORT, ECHO_PIN, GPIO_DIR_IN);
    GPIO_InputConfig(ECHO_PORT, ECHO_PIN, ENABLE); 
    
    /*-------- 定时器配置 --------*/
    // 启用SCTM0时钟
    memset(&CKCUClock, 0, sizeof(CKCUClock));
    CKCUClock.Bit.SCTM0 = 1;  
    CKCU_PeripClockConfig(CKCUClock, ENABLE);
   
    
    // 定时器初始化
    TIM_InitStruct.CounterReload = 10000-1;          // 自动重装载值
    TIM_InitStruct.Prescaler = 48-1;             // 预分频值
    TIM_InitStruct.RepetitionCounter = 0;
    TIM_InitStruct.CounterMode = TM_CNT_MODE_UP;
    TIM_InitStruct.PSCReloadTime = TM_PSC_RLD_IMMEDIATE;
    TM_TimeBaseInit(HT_SCTM0, &TIM_InitStruct);
    
    // 定时器中断
    TM_IntConfig(HT_SCTM0, TM_INT_UEV, DISABLE);
    // TM_ClearFlag(HT_SCTM0, TM_INT_UEV);         // 清除中断标志
    // TM_Cmd(HT_SCTM0, ENABLE);                   // 启动定时器
}

/***************************************************************
* @函数名：HCSR04_GetDistance（关键修正）
***************************************************************/
float HCSR04_GetDistance(void)
{
    float sum = 0;
    uint16_t valid = 0;
    uint8_t i;
    
    for(i = 0; i < SAMPLE_COUNT; i++)
    {
        uint32_t timeout;
        uint32_t start_cnt, end_cnt;
        
        /*-- 发送触发脉冲 --*/
        GPIO_ClearOutBits(TRIG_PORT, TRIG_PIN);
        Delay_us(2);
        GPIO_SetOutBits(TRIG_PORT, TRIG_PIN);
        Delay_us(20);
        GPIO_ClearOutBits(TRIG_PORT, TRIG_PIN);
        
        /*-- 等待回波上升沿 --*/
        timeout = MEASURE_TIMEOUT;
        while((GPIO_ReadInBit(ECHO_PORT, ECHO_PIN) == 0) && timeout--);
        if(timeout == 0) continue;
        
        /*-- 记录起始计数 --*/
        TM_Cmd(HT_SCTM0, ENABLE);
        HT_SCTM0->CNTR = 0; // 确保计数器从0开始
        
        /*-- 等待回波下降沿 --*/
        timeout = MEASURE_TIMEOUT;
        while((GPIO_ReadInBit(ECHO_PORT, ECHO_PIN) != 0) && timeout--);
        end_cnt = HT_SCTM0->CNTR; // 先读取计数值
        TM_Cmd(HT_SCTM0, DISABLE);
        
        if(timeout > 0)
        {
            float duration = end_cnt * 1e-6f; // 单位秒
            float distance = (duration * SOUND_SPEED) * 50; // 等效(duration*343)/2 * 100
            
            if(distance >= 2.0f && distance <= 400.0f) {
                sum += distance;
                valid++;
            }
        }
        Delay_ms(60);
    }
    return (valid > 0) ? (sum / valid) : -1.0f;
}
