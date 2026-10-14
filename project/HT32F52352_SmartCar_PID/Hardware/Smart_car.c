#include "ht32f5xxxx_01.h"              // Device header
#include "Motor.h"
#include "delay.h"
#include "Smart_car.h"
#include "GPTM0.h"
#include "GPTM1.h"
#include "math.h"
#include "BFTM0.h"

#define PI 3.1415926f

#define CONTROL_PERIOD  10    // 控制周期10ms
#define WHEEL_DIAMETER  6.5f  // 车轮直径(cm)
#define PPR             13    // 编码器每转脉冲数
#define GEAR_RATIO      45    // 减速比
#define FILTER_COEF    0.2f   // 速度滤波系数

static PID_Controller s_left_pos_pid = {.Kp=25, .Ki=0.1, .Kd=0, .max_integral=1000};
static PID_Controller s_left_speed_pid = {.Kp=1.2, .Ki=0.05, .Kd=0};
static PID_Controller s_right_pos_pid = {.Kp=25, .Ki=0.1, .Kd=0, .max_integral=1000};
static PID_Controller s_right_speed_pid = {.Kp=1.2, .Ki=0.05, .Kd=0};

static Car_State s_car_state = CAR_STOP;
static int32_t s_target_pulses = 0;
static float s_left_speed_filtered = 0;
static float s_right_speed_filtered = 0;

// 私有函数
static int32_t Calc_TargetPulses(uint32_t distance_cm);
static float Position_PID_Calculate(PID_Controller* pid, float error, float dt);
static void Motor_Output_Limit(int8_t* pwm);

/**
  * 函    数：小车初始化
  * 参    数：无
  * 返 回 值：无
  */
void SmartCar_Init(void)
{
    Motor_Init();
    BFTM0_Configuration();
    // 显式初始化位置环PID
    s_left_pos_pid.Kp = 25;
    s_left_pos_pid.Ki = 0.1;
    s_left_pos_pid.Kd = 0;
    s_left_pos_pid.max_integral = 1000;
    // 初始化PID积分项
    s_left_pos_pid.ErrorIntegral = s_right_pos_pid.ErrorIntegral = 0;
    s_left_speed_pid.ErrorIntegral = s_right_speed_pid.ErrorIntegral = 0;
}

/**
  * 函    数：小车前进
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void Move_Forward(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(speed);
    RightMotor_SetSpeed(speed);
}
/**
  * 函    数：小车后退
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void Move_Backward(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(-speed);
    RightMotor_SetSpeed(-speed);
}
/**
  * 函    数：小车右转
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void Move_Right(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(speed);      // 左轮高速
    RightMotor_SetSpeed(speed / 5); // 右轮低速
}
/**
  * 函    数：小车左转
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void Move_Left(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(speed / 5);  // 左轮低速
    RightMotor_SetSpeed(speed);     // 右轮高速
}
/**
  * 函    数：小车顺时针旋转
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void Clockwise_Rotation(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(speed);
    RightMotor_SetSpeed(-speed);
}
/**
  * 函    数：小车逆时针旋转
  * 参    数：speed - 速度值，范围 0~100
  * 返 回 值：无
  */
void CounterClockwise_Rotation(uint8_t speed)
{
    if (speed > 100) speed = 100;  // 限制速度范围
    LeftMotor_SetSpeed(-speed);
    RightMotor_SetSpeed(speed);
}
/**
  * 函    数：小车停止
  * 参    数：无
  * 返 回 值：无
  */
void Car_Stop(void)
{
    LeftMotor_SetSpeed(0);
    RightMotor_SetSpeed(0);
}

/**
  * 函    数：小车前进指定距离
  * 参    数：distance_cm - 距离值，单位 cm
  * 返 回 值：无
  */
void Move_Forward_Distance(uint32_t distance_cm)
{
    s_target_pulses = Calc_TargetPulses(distance_cm);
    Encoder_Reset();        // 左编码器硬件重置
    Encoder1_Reset();       // 右编码器硬件重置
    Reset_Encoders();       // 软件重置
    s_car_state = CAR_MOVING;
}

/**
  * 函    数：小车控制主循环
  * 参    数：无
  * 返 回 值：无  
  */
void Car_Control_Loop(void)
{
    static int32_t last_left = 0, last_right = 0;
    static uint32_t last_time = 0;
    
    if(s_car_state != CAR_MOVING) return;
    
    // 获取当前状态
    // int32_t left = Encoder_Get();
    // int32_t right = Encoder1_Get();
    int32_t left = left_encoder_total;  // 使用软件累计值
    int32_t right = right_encoder_total;

    uint32_t now = Get_SystemTick();
    float dt = (now - last_time) / 1000.0f;
    last_time = now;

    /* 位置环计算 */
    float left_pos_error = s_target_pulses - left;
    float right_pos_error = s_target_pulses - right;
    
    // 死区处理
    if(fabs(left_pos_error) < 5) left_pos_error = 0;
    if(fabs(right_pos_error) < 5) right_pos_error = 0;
    
    float target_left_speed = Position_PID_Calculate(&s_left_pos_pid, left_pos_error, dt);
    float target_right_speed = Position_PID_Calculate(&s_right_pos_pid, right_pos_error, dt);

    /* 速度环计算 */
    // 带滤波的速度计算
    float real_left_speed = (left - last_left) / dt;
    float real_right_speed = (right - last_right) / dt;
    s_left_speed_filtered = FILTER_COEF*real_left_speed + (1-FILTER_COEF)*s_left_speed_filtered;
    s_right_speed_filtered = FILTER_COEF*real_right_speed + (1-FILTER_COEF)*s_right_speed_filtered;
    last_left = left;
    last_right = right;
    
    float left_pwm = PID_Calculate(&s_left_speed_pid, target_left_speed-s_left_speed_filtered, dt);
    float right_pwm = PID_Calculate(&s_right_speed_pid, target_right_speed-s_right_speed_filtered, dt);

    /* 电机输出 */
    int8_t left_out = (int8_t)left_pwm;
    int8_t right_out = (int8_t)right_pwm;
    Motor_Output_Limit(&left_out);
    Motor_Output_Limit(&right_out);
    LeftMotor_SetSpeed(left_out);
    RightMotor_SetSpeed(right_out);

    /* 完成检测 */
    if(left >= s_target_pulses && right >= s_target_pulses) {
        Car_Stop();
        s_car_state = CAR_STOP;
    }
}

// 计算目标脉冲数（考虑四倍频和减速比）
static int32_t Calc_TargetPulses(uint32_t distance_cm)
{
    const float circumference = PI * WHEEL_DIAMETER;
    return (int32_t)((distance_cm / circumference) * PPR * 4 * GEAR_RATIO);
}

// PID计算函数（带积分限幅）
static float Position_PID_Calculate(PID_Controller* pid, float error, float dt)
{
    // 积分项处理
    pid->ErrorIntegral += error * dt;
    pid->ErrorIntegral = fmaxf(fminf(pid->ErrorIntegral, pid->max_integral), -pid->max_integral);
    
    // 微分项处理
    float derivative = (error - pid->LastError) / dt;
    
    // PID输出
    float output = pid->Kp * error + 
                  pid->Ki * pid->ErrorIntegral + 
                  pid->Kd * derivative;
    
    pid->LastError = error;
    return output;
}

// 输出限幅（-100~100）
static void Motor_Output_Limit(int8_t* pwm)
{
    if(*pwm > 100)       *pwm = 100;
    else if(*pwm < -100) *pwm = -100;
}
