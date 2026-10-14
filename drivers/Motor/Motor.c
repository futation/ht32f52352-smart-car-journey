#include "ht32f5xxxx_01.h"              // 设备头文件
#include "MCTM0.h"
#include "stdlib.h"
#include "motor.h"
#include "GPTM0.h"
#include "GPTM1.h"

static PID_Controller leftPID = {.Kp=0.2, .Ki=0.00015, .Kd=0};
static PID_Controller rightPID = {.Kp=0.2, .Ki=0.00015, .Kd=0};
int32_t left_encoder_total = 0;
int32_t right_encoder_total = 0;
static int32_t last_left_encoder = 0, last_right_encoder = 0;

// PID初始化
void PID_Init(PID_Controller* pid, float kp, float ki, float kd) 
{
  pid->Kp = kp;
  pid->Ki = ki;
  pid->Kd = kd;
  pid->ErrorIntegral = 0;
  pid->LastError = 0;
  pid->TargetSpeed = 0;
  pid->ActualSpeed = 0;
  pid->max_integral = 1000; // 积分上限
}

// PID计算
int PID_Calculate(PID_Controller* pid, int target, int actual) 
{
  float error,derivative,output;

  error = target - actual;
  pid->ErrorIntegral += error;
  if (pid->ErrorIntegral > pid->max_integral) 
      pid->ErrorIntegral = pid->max_integral;
  if (pid->ErrorIntegral < -pid->max_integral) 
      pid->ErrorIntegral = -pid->max_integral;
  derivative = error - pid->LastError;
  output = (pid->Kp * error) + (pid->Ki * pid->ErrorIntegral) +(pid->Kd * derivative);
  pid->LastError = error;
  return (int)output;
}

// 修改编码器读取函数
int Read_LeftEncoder(void) 
{
    int32_t current = Encoder_Get();
    int32_t delta = current - last_left_encoder;
    last_left_encoder = current;
    left_encoder_total += delta; // 保留方向信息
    return delta;
}

int Read_RightEncoder(void) 
{
    int32_t current = Encoder1_Get();
    int32_t delta = current - last_right_encoder;
    last_right_encoder = current;
    right_encoder_total += delta; // 保留方向信息
    return delta;
}

// 重置编码器
void Reset_Encoders(void) 
{
    left_encoder_total = 0;
    right_encoder_total = 0;
}

 // 电机初始化
void Motor_Init(void)
{
    // 初始化外设时钟和GPIO配置
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{ 0 }};
    // 使能GPIOA和AFIO外设时钟
    CKCUClock.Bit.PA    = 1;     // 启用GPIOA时钟
    CKCUClock.Bit.PC    = 1;     // 启用GPIOC时钟
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // 配置PC2和PC9为推挽输出模式，初始电平为低
    GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_2, GPIO_DIR_OUT); // PC2设为输出模式
    GPIO_ClearOutBits(HT_GPIOC, GPIO_PIN_2);      // PC2置低电平

    GPIO_DirectionConfig(HT_GPIOC, GPIO_PIN_9, GPIO_DIR_OUT); // PC9设为输出模式
    GPIO_ClearOutBits(HT_GPIOC, GPIO_PIN_9);      // PC9置低电平

    // 配置PA8和PA9为推挽输出模式，初始电平为低
    GPIO_DirectionConfig(HT_GPIOA, GPIO_PIN_8, GPIO_DIR_OUT); // PA8设为输出模式
    GPIO_ClearOutBits(HT_GPIOA, GPIO_PIN_8);      // PA8置低电平

    GPIO_DirectionConfig(HT_GPIOA, GPIO_PIN_9, GPIO_DIR_OUT); // PA9设为输出模式
    GPIO_ClearOutBits(HT_GPIOA, GPIO_PIN_9);      // PA9置低电平

    // 初始化MCTM0定时器（替代原有的PWM_Init）
    MCTM0_Configuration();  // 配置定时器和PWM输出功能
    GPTM0_Configuration(); // 配置GPTM0时钟和GPIO配置
    GPTM1_Configuration(); // 配置GPTM1时钟和GPIO配置
    PID_Init(&leftPID, 0.2, 0.00015, 0);
    PID_Init(&rightPID, 0.2, 0.00015, 0);
}

/**    
  * 函    数：小车左轮设置速度
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
void LeftMotor_SetSpeed(int8_t Speed)
{
  leftPID.TargetSpeed = Speed;  // 设置目标速度
}

/**    
  * 函    数：小车右轮设置速度
  * 参    数：Speed 要设置的速度，范围：-100~100
  * 返 回 值：无
  */
  void RightMotor_SetSpeed(int8_t Speed)
  {
    rightPID.TargetSpeed = Speed; // 设置目标速度
  }

// 速度更新函数（需要定期调用）
void Motor_UpdateSpeed(void) 
{
  int left_output,right_output;
  // 假设调用间隔为10ms
  float time_interval = 0.01; // 单位：秒
  leftPID.ActualSpeed = Read_LeftEncoder() / time_interval; // 转换为脉冲/秒
  rightPID.ActualSpeed = Read_RightEncoder() / time_interval; // 转换为脉冲/秒
  
  // 计算PID输出
  left_output = PID_Calculate(&leftPID, leftPID.TargetSpeed,leftPID.ActualSpeed);
                                                        
  right_output = PID_Calculate(&rightPID,rightPID.TargetSpeed,rightPID.ActualSpeed);
                                 
                                 
  // 应用PWM输出（需要限制范围）
  left_output = (left_output < -100) ? -100 : (left_output > 100) ? 100 : left_output;
  right_output = (right_output < -100) ? -100 : (right_output > 100) ? 100 : right_output;
  
  // 调用原始PWM设置函数
  if (left_output >= 0) 
  {
		  GPIO_SetOutBits(HT_GPIOC, GPIO_PIN_2);	// PC2置高电平
		  GPIO_ClearOutBits(HT_GPIOC, GPIO_PIN_9);	// PC9置低电平，设置方向为正转
      MCTM0_CH3_SetOnduty(left_output);
  } 
  else 
  {
      GPIO_SetOutBits(HT_GPIOC, GPIO_PIN_9);	// PC9置高电平
		  GPIO_ClearOutBits(HT_GPIOC, GPIO_PIN_2);	// PC2置低电平，设置方向为反转
      MCTM0_CH3_SetOnduty(-left_output);
  }
  
  if (right_output >= 0) 
  {
    	GPIO_SetOutBits(HT_GPIOA, GPIO_PIN_9);	// PA9置高电平
		  GPIO_ClearOutBits(HT_GPIOA, GPIO_PIN_8);	// PA8置低电平，设置方向为反转
      MCTM0_CH2_SetOnduty(right_output);
  } 
  else 
  {
    	GPIO_SetOutBits(HT_GPIOA, GPIO_PIN_8);	// PA8置高电平
		  GPIO_ClearOutBits(HT_GPIOA, GPIO_PIN_9);	// PA9置低电平，设置方向为正转
      MCTM0_CH2_SetOnduty(-right_output);
  }
}
