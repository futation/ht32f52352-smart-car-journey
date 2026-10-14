#ifndef __GPTM0_H
#define __GPTM0_H

#ifdef __cplusplus
 extern "C" {
#endif

//-----------------------------------------------------------------------------
#include "ht32.h"         // Holtek 标准库头文件
//-----------------------------------------------------------------------------
/**? 
  * @brief 编码器初始化函数
  * @note 配置 GPTM0 为正交编码器模式（PA4/PA5 作为输入）
  */
void GPTM0_Configuration(void);
void Encoder_Reset(void);
/**
  * @brief 读取编码器计数值
  * @retval 当前32位计数器值
  **/
u32 Encoder_Get(void);

#ifdef __cplusplus
}
#endif

#endif // __GPTM0_H
