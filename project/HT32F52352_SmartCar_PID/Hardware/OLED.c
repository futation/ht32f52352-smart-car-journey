#include "ht32.h"
#include "OLED_Font.h"

// 引脚定义
#define OLED_SCL_GPIO_PORT    HT_GPIOC
#define OLED_SCL_GPIO_PIN     GPIO_PIN_12
#define OLED_SDA_GPIO_PORT    HT_GPIOC
#define OLED_SDA_GPIO_PIN     GPIO_PIN_11

// 引脚操作宏
#define OLED_W_SCL(x)        ((x) ? GPIO_SetOutBits(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN) : GPIO_ClearOutBits(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN))
#define OLED_W_SDA(x)        ((x) ? GPIO_SetOutBits(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN) : GPIO_ClearOutBits(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN))

/* 引脚初始化 */
void OLED_I2C_Init(void)
{
    // 使能GPIOC时钟
    CKCU_PeripClockConfig_TypeDef CKCUClock = {{0}};
    CKCUClock.Bit.PC = 1;
    CKCU_PeripClockConfig(CKCUClock, ENABLE);

    // 配置SCL引脚
    GPIO_DirectionConfig(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, GPIO_DIR_OUT);
    GPIO_InputConfig(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, DISABLE);
    //GPIO_OpenDrainConfig(OLED_SCL_GPIO_PORT, OLED_SCL_GPIO_PIN, ENABLE);

    // 配置SDA引脚
    GPIO_DirectionConfig(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, GPIO_DIR_OUT);
    GPIO_InputConfig(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, DISABLE);
    //GPIO_OpenDrainConfig(OLED_SDA_GPIO_PORT, OLED_SDA_GPIO_PIN, ENABLE);

    // 初始电平置高
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* I2C起始信号 */
void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_W_SDA(0);
    OLED_W_SCL(0);
}

/* I2C停止信号 */
void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

/* I2C发送一个字节 */
void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for(i = 0; i < 8; i++)
    {
        OLED_W_SDA(!!(Byte & (0x80 >> i)));
        OLED_W_SCL(1);
        OLED_W_SCL(0);
    }
    OLED_W_SCL(1);  // 第九个时钟脉冲
    OLED_W_SCL(0);
}

/* OLED写命令 */
void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);  // 从机地址+写标志
    OLED_I2C_SendByte(0x00);   // 控制字节-命令
    OLED_I2C_SendByte(Command);
    OLED_I2C_Stop();
}

/* OLED写数据 */
void OLED_WriteData(uint8_t Data)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78);   // 从机地址+写标志
    OLED_I2C_SendByte(0x40);   // 控制字节-数据
    OLED_I2C_SendByte(Data);
    OLED_I2C_Stop();
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);		//设置光标位置在上半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);			//显示上半部分内容
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);	//设置光标位置在下半部分
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);		//显示下半部分内容
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED显示浮点数（带小数位）
  * @param  Line      行位置，范围：1~4
  * @param  Column    起始列位置，范围：1~16
  * @param  Number    要显示的浮点数
  * @param  IntLen    整数部分显示位数（含符号位）
  * @param  DecPlaces 小数位数，范围：1~5
  * @retval 无
  * @说明：
  * 1. 显示格式：[符号][整数].[小数]
  * 2. 自动四舍五入到指定小数位
  * 3. 示例：OLED_ShowFloat(1, 1, -123.456, 4, 2) → 显示"-123.46"
  */
void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Number, uint8_t IntLen, uint8_t DecPlaces)
{
    uint8_t current_col = Column;
    uint32_t int_part, dec_part;
    uint32_t scale = OLED_Pow(10, DecPlaces);  // 使用现有次方函数
    uint32_t scaled_val;
    /* 参数保护 */
    DecPlaces = (DecPlaces > 5) ? 5 : DecPlaces;  // 最大支持5位小数
    
    /* 符号处理 */
    if(Number < 0){
        OLED_ShowChar(Line, current_col, '-');
        current_col++;
        Number = -Number;
        IntLen--;  // 符号位占用了整数位数
    }
    
    /* 四舍五入和数值拆分 */
    scaled_val = (uint32_t)(Number * scale + 0.5f); // 四舍五入
    int_part = scaled_val / scale;            // 整数部分
    dec_part = scaled_val % scale;             // 小数部分
    
    /* 显示整数部分（带前导空格对齐）*/
    OLED_ShowNum(Line, current_col, int_part, IntLen);
    current_col += IntLen;
    
    /* 显示小数点 */
    OLED_ShowChar(Line, current_col, '.');
    current_col++;
    
    /* 显示小数部分（自动补零）*/
    OLED_ShowNum(Line, current_col, dec_part, DecPlaces);
}

/**
  * @brief  OLED显示浮点数（优化布局版，自动计算显示位）
  * @param  Line      行位置，范围：1~4
  * @param  Column    起始列位置，范围：1~16
  * @param  Number    要显示的浮点数
  * @param  TotalLen  总显示位数（含符号和小数点）
  * @param  DecPlaces 小数位数
  * @retval 无
  * @示例：OLED_ShowFloatEx(1, 1, -12.345, 7, 2) → "-12.35"
  */
void OLED_ShowFloatEx(uint8_t Line, uint8_t Column, float Number, uint8_t TotalLen, uint8_t DecPlaces)
{
    uint8_t sign = (Number < 0);
    uint8_t int_len = TotalLen - DecPlaces - 1 - sign; // 计算整数显示位
    
    // 调用基础浮点显示函数
    OLED_ShowFloat(Line, Column, Number, int_len, DecPlaces);
}


/* OLED初始化 */
void OLED_Init(void)
{
    // 硬件初始化
    OLED_I2C_Init();

    // 初始化序列
    OLED_WriteCommand(0xAE); 	// 关闭显示
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF); 	// 开启显示
	
    OLED_Clear();
}
