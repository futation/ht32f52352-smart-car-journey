/* Servo.h */
#ifndef __SERVO_H
#define __SERVO_H

#include "ht32.h"

// 舵机角度范围（0°-180°）
#define SERVO_MIN_ANGLE     0
#define SERVO_MAX_ANGLE     180

void Servo_Init(void);
void Servo_SetAngle(uint8_t angle);
void Servo_Enable(void);
void Servo_Disable(void);

#endif /* __SERVO_H */
