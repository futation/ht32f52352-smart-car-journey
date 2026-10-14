#ifndef __SMARTCAR_H
#define __SMARTCAR_H

/* SmartCar.h */
#pragma once
#include "ht32f5xxxx_01.h"
#include "Motor.h"


typedef enum {
    CAR_STOP,
    CAR_MOVING,
    CAR_HOLD
} Car_State;

void SmartCar_Init(void);

void Move_Forward(uint8_t speed);

void Move_Backward(uint8_t speed);

void Move_Right(uint8_t speed);

void Move_Left(uint8_t speed);

void Clockwise_Rotation(uint8_t speed);

void CounterClockwise_Rotation(uint8_t speed);

void Car_Stop(void);

void Move_Forward_Distance(uint32_t distance_cm);

void Car_Control_Loop(void);

extern int speed;  // 声明为外部变量
extern int32_t left_encoder_total ;
extern int32_t right_encoder_total ;

#endif // __SMARTCAR_H
