//-----------------------------------------------------------------------------
#ifndef __GPIO_H
#define __GPIO_H

#include "ht32.h"

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
#define OUT_PC14_PORT           C
#define OUT_PC14_PIN            14
#define OUT_PC14_GPIO_CLK      STRCAT2(P,          OUT_PC14_PORT)
#define OUT_PC14_GPIO_ID       STRCAT2(GPIO_P,     OUT_PC14_PORT)
#define OUT_PC14_GPIO_PORT     STRCAT2(HT_GPIO,    OUT_PC14_PORT)
#define OUT_PC14_GPIO_PIN      STRCAT2(GPIO_PIN_,  OUT_PC14_PIN)
#define OUT_PC14_AFIO_MODE     (AFIO_MODE_GPIO)
#define OUT_PC14_HIGH          OUT_PC14_GPIO_PORT->SRR    = OUT_PC14_GPIO_PIN
#define OUT_PC14_LOW           OUT_PC14_GPIO_PORT->RR     = OUT_PC14_GPIO_PIN
#define OUT_PC14_XOR           OUT_PC14_GPIO_PORT->DOUTR ^= OUT_PC14_GPIO_PIN



//-----------------------------------------------------------------------------
void LED_Configuration(void);

#endif
