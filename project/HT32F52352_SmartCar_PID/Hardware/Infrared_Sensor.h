/* Infrared_Sensor.h */
#ifndef __INFRARED_SENSOR_H
#define __INFRARED_SENSOR_H

#ifdef __cplusplus
 extern "C" 
	 {
#endif

#include "ht32.h"

// 引脚定义
#define LEFT_INFRARED_PORT    HT_GPIOA
#define LEFT_INFRARED_PIN     GPIO_PIN_15
#define RIGHT_INFRARED_PORT   HT_GPIOA
#define RIGHT_INFRARED_PIN    GPIO_PIN_14

void Infrared_Sensor_Init(void);
uint8_t Read_Left_Infrared_Sensor(void);
uint8_t Read_Right_Infrared_Sensor(void);

#ifdef __cplusplus
}
#endif

#endif /* __INFRARED_SENSOR_H */
