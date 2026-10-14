#ifndef __MOTOR_H
#define __MOTOR_H

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    int TargetSpeed;        //目标速度
    int ActualSpeed;        //实际速度
    float ErrorIntegral;    //积分误差
    float LastError;        //上一次误差
    float max_integral;    // 新增积分限幅
} PID_Controller;

void Motor_Init(void);
void LeftMotor_SetSpeed(int8_t Speed);
void RightMotor_SetSpeed(int8_t Speed);
// PID相关函数
void PID_Init(PID_Controller* pid, float kp, float ki, float kd);
int PID_Calculate(PID_Controller* pid, int target, int actual);
void Motor_UpdateSpeed(void);

extern PID_Controller leftPID, rightPID;
extern int32_t left_encoder_total, right_encoder_total;
void Reset_Encoders(void);

// 新增编码器接口
int Read_LeftEncoder(void);
int Read_RightEncoder(void);
#endif
