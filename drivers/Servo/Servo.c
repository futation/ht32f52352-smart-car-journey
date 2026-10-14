/* Servo.c - 修改为使用SCTM1定时器，PB3引脚输出PWM */
#include "Servo.h"

// SG90舵机PWM参数
#define PWM_FREQUENCY       50      // 50Hz
#define MIN_PULSE_MS        0.5f    // 0度对应脉冲宽度
#define MAX_PULSE_MS        2.5f    // 180度对应脉冲宽度
#define PWM_PERIOD_MS       20.0f   // 20ms周期

static uint32_t pwm_period_ticks = 0; // 保存周期计数值

void Servo_Init(void)
{
    TM_TimeBaseInitTypeDef TM_TimeBaseInitStruct;
    TM_OutputInitTypeDef TM_OutputInitStruct;
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    uint32_t timer_clock, prescaler = 0, period;

    // 使能SCTM1、PB、AFIO时钟
    CKCUClock.Bit.PB    = 1;
    CKCUClock.Bit.SCTM1 = 1;
    CKCUClock.Bit.AFIO  = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // 配置PB3为SCTM1 PWM输出（通道0）
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_3, AFIO_FUN_SCTM);

    // 计算定时器参数（50Hz）
    timer_clock = SystemCoreClock; // SCTM1时钟源为系统时钟
    period = (timer_clock / PWM_FREQUENCY) - 1;

    // 计算预分频值和ARR
    while ((period / (prescaler + 1)) > 0xFFFF) 
	{
        prescaler++;
    }
    uint32_t arr = period / (prescaler + 1);
    pwm_period_ticks = arr + 1; // 实际周期计数值

    // 配置时基
    TM_TimeBaseInitStruct.CounterReload   = arr;
    TM_TimeBaseInitStruct.Prescaler       = prescaler;
    TM_TimeBaseInitStruct.RepetitionCounter = 0;
    TM_TimeBaseInitStruct.CounterMode     = TM_CNT_MODE_UP;
    TM_TimeBaseInitStruct.PSCReloadTime   = TM_PSC_RLD_IMMEDIATE;
    TM_TimeBaseInit(HT_SCTM1, &TM_TimeBaseInitStruct);

    // 配置PWM通道0（PB3）
    TM_OutputInitStruct.Channel        = TM_CH_0;     // 使用通道0
    TM_OutputInitStruct.OutputMode     = TM_OM_PWM1;  // 注意模式选择
    TM_OutputInitStruct.Control        = TM_CHCTL_ENABLE;
    TM_OutputInitStruct.Polarity       = TM_CHP_NONINVERTED; // 非反向极性
    TM_OutputInitStruct.Compare        = 0; // 初始占空比0
    TM_OutputInit(HT_SCTM1, &TM_OutputInitStruct);

    // 使能定时器
    TM_Cmd(HT_SCTM1, ENABLE);
}

void Servo_SetAngle(uint8_t angle)
{
    float pulse_width;
    uint16_t ccr;

    if(angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;

    // 计算脉冲宽度（0.5ms~2.5ms）
    pulse_width = MIN_PULSE_MS + (MAX_PULSE_MS - MIN_PULSE_MS) * 
                   (angle / (float)SERVO_MAX_ANGLE);

    // 计算比较值（CCR）
    ccr = (uint16_t)((pulse_width / PWM_PERIOD_MS) * pwm_period_ticks);
    TM_SetCaptureCompare0(HT_SCTM1, ccr); // 设置通道0的比较值
}

void Servo_Enable(void)
{
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_3, AFIO_FUN_SCTM); // 恢复PWM输出
    TM_Cmd(HT_SCTM1, ENABLE);
}

void Servo_Disable(void)
{
    // 将PB3设为GPIO并输出低电平
    GPIO_ClearOutBits(HT_GPIOB, AFIO_PIN_3);
    GPIO_DirectionConfig(HT_GPIOB, AFIO_PIN_3, GPIO_DIR_OUT);
    AFIO_GPxConfig(GPIO_PB, AFIO_PIN_3, AFIO_FUN_GPIO); // 切换回GPIO模式
    TM_Cmd(HT_SCTM1, DISABLE);
}
