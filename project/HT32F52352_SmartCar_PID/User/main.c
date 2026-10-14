#include "ht32.h"
#include "ht32_board.h"

#include <stdint.h> // 提供标准整数类型定义(如uint8_t)
#include <string.h> // 提供字符串操作函数
#include <math.h>   // 数学函数库(如fabs)

#include "LED.h"
#include "delay.h"
#include "OLED.h"
#include "UART0.h"
#include "Smart_car.h"
#include "Servo.h"
#include "HCSR04.h"         // 超声波测距模块
#include "Infrared_Sensor.h"// 红外避障传感器
#include "Scaner.h"         // 扫码模块

#include "Noblo_Delay.h"   //非阻塞延时(包含BFTM0)
#include "BFTM1.h"         // 基础定时器1（PID周期）
#include "Motor.h"         // 电机驱动PID控制

/********************* 新增状态机结构体 *******************/
typedef enum {
    STATE_IDLE,    // 空闲状态
    STATE_MOVING,  // 运动状态
    STATE_TURNING, // 转向状态
    STATE_STOPPING, // 停止状态
    STATE_RETURNING // 返回状态
} MotionState;

typedef struct {
    DelayTimer moveTimer;   // 运动延时计时器
    DelayTimer turnTimer;   // 转向延时计时器
    DelayTimer stopTimer;   // 停止延时计时器
    MotionState state;      // 当前状态
    int savedForward;       // 保存的前进格数
    int savedFace;          // 保存的转向方向
    int savedMove;          // 保存的后退格数
} MotionControl;

/********************* 全局变量定义 **********************/
int number = 0;             // 通用计数器
int speed = 50;             // 默认运动速度（范围建议20-100）

int Forward = 0;           //前进的格数
int face =0;               //前进后向左或者向右
int Move=0;                //左移或者右移的格数
int gril=1000;               //栅格参数_这里是时间

float Distance = 0.0f;       // 前方障碍物距离（单位：厘米）
float Left_Distance = 0.0f;  // 左侧障碍物距离
float Right_Distance = 0.0f; // 右侧障碍物距离
float Angle = 90.0f;         // 舵机初始角度（正前方）


volatile uint8_t isMoving = 0; // 运动状态标志（0：停止，1：运动中）
static int Turn_time = 650;//转向时间

char g_scanData[15] = {0}; // 存储扫码结果字符串
char g_Bluetooth[15]={0};  // 储存蓝牙接受结果字符串。

MotionControl motion = {
    .state = STATE_IDLE
};
DelayTimer obstacleTimer;  // 避障延时计时器
DelayTimer measureTimer;   // 测距延时计时器

// 在全局变量区添加PID相关变量
// 右电机PID参数及状态量
int TargetR = 0;           // 目标转速（编码器脉冲数/周期）
int ActualR = 0;           // 实际转速测量值
int OutR = 0;              // PID输出控制量
float KpR = 0.20;          // 比例系数
float KiR = 0.00015;       // 积分系数
float KdR = 0;             // 微分系数
float Error0R, Error1R = 0, ErrorIntR = 0;
int speedR = 0;            // 当前转速缓存

/********************* 函数声明 ?*********************/
void DecideDirection(void);     // 障碍规避方向决策
void MeasureDistance(void);     // 左右距离测量
void Avoid_Obstacle(void);      // 红外避障处理
void Ultrasonic_Avoidance(void); // 超声波避障主逻辑
void Bluetooth_Driver(void);    // 蓝牙指令处理
void Scaner_Get(void);          // 扫码模块数据获取
void Scaner_stop(void);
void MotionStateMachine(void);  // 状态机处理
/********************* 主函数 ?*********************/
int main()
{
#if (HTCFG_STACK_USAGE_ANALYSIS == 1)
    StackUsageAnalysisInit(0);  // 堆栈使用分析（调试用）
#endif

    /****** 外设初始化 ?******/
    OLED_Init();            // 初始化OLED显示屏
	Delay_Init();			//非阻塞初始化
    UART0_Configuration(); // 配置UART0串口（蓝牙通信）
    LED_Configuration();   // 初始化板载LED
    SmartCar_Init();        // 初始化电机驱动
    Servo_Init();           // 初始化舵机控制
    HCSR04_Init();          // 初始化超声波模块
    Infrared_Sensor_Init(); // 初始化红外传感器
    USART1_Configuration(); // 配置USART1（扫码模块）
    BFTM1_Configuration();     // 基础定时器1配置（PID周期）
    // 串口发送启动信息
    UART0_SendString("Begining!\r\n");

    // 初始化舵机角度至正前方
    Servo_SetAngle(Angle);

    // 显示初始速度值
    OLED_ShowString(2, 1, "Speed:");
    OLED_ShowNum(2, 7, speed, 3);
    
    Move_Forward_Distance(10); // 前进10cm

    /**?*?**?*?**?*?**?*?**?*?**?*?**?* 主循环逻辑 ?**?*?**?*?**?*?**?*?**?*?**?*?**?*/
    while (1)
    {
        // 看门狗维护（如果启用）
        if (HT_CKCU->APBCCR1 & (1 << 4))
            WDT_Restart();

        // 在OLED第一行显示计数器值
        OLED_ShowNum(1, 1, number, 5);

        /****** 功能模块调用 ?******/
        Bluetooth_Driver();      // 处理蓝牙控制指令
        MotionStateMachine();    // 状态机处理
        Scaner_Get();            // 处理扫码模块数据
		Car_Control_Loop();      // 电机控制主逻辑
        
        // 运动状态下的避障系统
        if(isMoving == 1)
        {
            Avoid_Obstacle();        // 执行红外避障策略
            Ultrasonic_Avoidance();  // 执行超声波避障策略
            Scaner_stop();           // 扫码对比，须在发送路劲之后发送编码
        }
    }
}

/********************* 蓝牙指令处理函数 ?*********************/
void Bluetooth_Driver(void)
{
    if (Serial_RxFlag)  // 检测到完整数据包
    {
        // 类型转换：将接收缓冲区转换为字符串格式
        char *rxData = (char *)Serial_RxPacket;

        // 保存扫码数据
        strncpy(g_Bluetooth, (const char *)Serial_RxPacket, 15-1);
        g_Bluetooth[15-1] = '\0'; // 确保字符串终止

        // 串口回显接收内容
        UART0_SendString("Received: ");
        UART0_SendString(Serial_RxPacket);
		UART0_SendString("\r\n");

        // OLED显示接收内容（第3行）
		OLED_ShowString(3, 4, "         ");
        OLED_ShowString(3, 1, "RX:");
        OLED_ShowString(3, 4, rxData);

        /*---- 指令解析与执行 ----*/
        // 方向控制指令
        if (strcmp(rxData, "@Forward.") == 0)
        {
            Move_Forward(speed);
            isMoving = 1; // 进入运动状态
        }
        else if (strcmp(rxData, "@Backward.") == 0)
        {
            Move_Backward(speed);
            isMoving = 1; // 进入运动状态
        }
        else if (strcmp(rxData, "@Left.") == 0)
        {
            Move_Left(speed);
            isMoving = 1; // 进入运动状态
        }
        else if (strcmp(rxData, "@Right.") == 0)
        {
            Move_Right(speed);
            isMoving = 1; // 进入运动状态
        }
        // 旋转控制指令
        else if (strcmp(rxData, "@CWRotate.") == 0)
        {
            Clockwise_Rotation(speed);
            isMoving = 1; // 进入运动状态
        }
        else if (strcmp(rxData, "@CCWRotate.") == 0)
        {
            CounterClockwise_Rotation(speed);
            isMoving = 1; // 进入运动状态
        }
        // 停止指令
        else if (strcmp(rxData, "@Stop.") == 0)
        {
            Car_Stop();
            isMoving = 0; // 退出运动状态
        }
        // 速度设置指令（格式：@SetSpeed:数值.）
        else if(sscanf((const char *)Serial_RxPacket, "@SetSpeed:%d.", &speed) == 1)
        {
            // 速度范围限制（20-100）
            speed = (speed < 20) ? 20 : 
                   (speed > 100) ? 100 : speed;
            
            // OLED更新速度显示
            OLED_ShowString(2, 1, "Speed:    "); // 清空旧值
            OLED_ShowNum(2, 7, speed, 3);
            
            // 串口反馈设置结果
            UART0_SendString("Speed set to: ");
            UART0_SendNumber(speed,3);
            UART0_SendString("\r\n");
        }
        //路径指令接收
        else if (sscanf(rxData, "@F%d_%d_M%d.", &motion.savedForward, &motion.savedFace, &motion.savedMove) == 3)
        {
            motion.state = STATE_MOVING;
            Move_Forward(speed);
            Delay_NonBlocking(&motion.moveTimer, motion.savedForward * gril);
        }

        // 返回指令处理（修改后）
        else if (strcmp(rxData, "@Return.") == 0)
        {
            motion.state = STATE_RETURNING;
            Move_Backward(speed);
            Delay_NonBlocking(&motion.moveTimer, motion.savedMove * gril);
        }

        // 未知指令处理
        else
        {
            UART0_SendString("Unknown Command!");
        }

        Serial_RxFlag = 0; // 清除接收标志
    }
}

/********************* 新增状态机处理函数 ?**?*?**?*?**?*?**?*?**?*?**?*?**?*/
void MotionStateMachine(void)
{
    switch(motion.state)
    {
        case STATE_MOVING:   // 前进状态
            //检查非阻塞定时器是否到期
            if(Delay_NonBlocking(&motion.moveTimer, motion.savedForward * gril))
            {
				// 停止后必须重置运动标志
                Car_Stop();
                isMoving = 0;
				// 判断是否为最后一次移动（检查savedMove）
				if(motion.savedMove == 0) 
				{
					Car_Stop();
					motion.state = STATE_IDLE;
					isMoving = 0;
					break;
				}

                // 根据保存的转向标识执行旋转
                if(motion.savedFace == 1)// 左转标识
                    CounterClockwise_Rotation(speed);
                else if(motion.savedFace == 2)// 右转标识
                    Clockwise_Rotation(speed);
                    
                // 进入转向阶段并设置转向定时器
                motion.state = STATE_TURNING;
                Delay_NonBlocking(&motion.turnTimer, Turn_time);
            }
            break;
            
        case STATE_TURNING: // 转向执行阶段
            if(Delay_NonBlocking(&motion.turnTimer, Turn_time))
            {
                Car_Stop(); // 停止转向动作
                motion.state = STATE_STOPPING; // 进入缓冲停止阶段
                Delay_NonBlocking(&motion.stopTimer, 200); // 200ms稳定期
            }
            break;
            
        case STATE_STOPPING:// 转向后稳定阶段
            if(Delay_NonBlocking(&motion.stopTimer, 200))
            {   
				 // 先检查是否需要终止
                if(motion.savedMove == 0) 
                {
                    Car_Stop();
                    motion.state = STATE_IDLE;
                    isMoving = 0;
                    break;
                }
                // 执行最后一段移动（savedMove）
				// 启动二次移动前保存参数
                int finalMove = motion.savedMove;
                motion.savedMove = 0; // 标记为最后一次
				motion.savedForward = finalMove;
                Move_Forward(speed);
                Delay_NonBlocking(&motion.moveTimer, motion.savedForward * gril);
                motion.state = STATE_MOVING;
				isMoving = 1; // 必须重新激活运动标志
            }
            break;
            
        case STATE_RETURNING: // 返回路径阶段
            if(Delay_NonBlocking(&motion.moveTimer, motion.savedMove * gril))
            {
                // 逆向执行原始转向逻辑
                if(motion.savedFace == 1) // 原左转改为右转
                    Clockwise_Rotation(speed);
                else if(motion.savedFace == 2) // 原右转改为左转
                    CounterClockwise_Rotation(speed);
                
                // 进入转向阶段
                motion.state = STATE_TURNING;
                Delay_NonBlocking(&motion.turnTimer, Turn_time);
            }
            break;
            
        default:// 空闲状态不做处理
            break;
    }
}


/********************* 红外避障函数 ?*********************/
void Avoid_Obstacle(void)
{
    // 读取传感器状态（0表示检测到障碍物）
    uint8_t left_obstacle = Read_Left_Infrared_Sensor();
    uint8_t right_obstacle = Read_Right_Infrared_Sensor();

	if(motion.state != STATE_IDLE) return; // 状态机运行时禁用避障
	
    /* 避障策略逻辑 */
    // 两侧均有障碍
    if (left_obstacle == 0 && right_obstacle == 0)
    {
        Car_Stop();
        Move_Backward(speed);   // 倒车脱困
        Delay_ms(500);
        Clockwise_Rotation(speed); // 顺时针旋转
        Delay_ms(300);
    }
    // 左侧障碍
    else if (left_obstacle == 0)
    {
        Move_Left(speed); // 左转避让
    }
    // 右侧障碍
    else if (right_obstacle == 0)
    {
        Move_Right(speed); // 右转避让
    }
}

/********************* 超声波避障主逻辑 ?*********************/
void Ultrasonic_Avoidance(void)
{
    if(motion.state != STATE_IDLE) return; // 状态机运行时禁用避障

    // 获取前方障碍物距离
    Distance = HCSR04_GetDistance();

    // 危险距离判断（10厘米）
    if (Distance <= 10.0f)
    {
        Car_Stop();
        MeasureDistance();   // 测量左右距离
        DecideDirection();
        Delay_ms(500);
    }
    else
    {
        Servo_SetAngle(Angle); // 恢复舵机至正前方
    }
}

/********************* 左右距离测量函数 ?*********************/
void MeasureDistance(void)
{
    static uint8_t measureStep = 0;
    
    switch(measureStep)
    {
        case 0:
            Servo_SetAngle(Angle-60.0f);
            Delay_NonBlocking(&measureTimer, 600);
            measureStep = 1;
            break;
            
        case 1:
            if(Delay_NonBlocking(&measureTimer, 600))
            {
                Right_Distance = HCSR04_GetDistance();
                Servo_SetAngle(Angle+60.f);
                Delay_NonBlocking(&measureTimer, 600);
                measureStep = 2;
            }
            break;
            
        case 2:
            if(Delay_NonBlocking(&measureTimer, 600))
            {
                Left_Distance = HCSR04_GetDistance();
                Servo_SetAngle(Angle);
                measureStep = 0;
            }
            break;
    }
}

/********************* 规避方向决策函数 ?*********************/
void DecideDirection(void)
{
    const float threshold = 5.0f; // 有效差距阈值（单位：厘米）

    // 比较左右距离差
    if (fabs(Right_Distance - Left_Distance) < threshold)
    {
        // 差距不足时原地旋转
        Clockwise_Rotation(speed);
        Delay_ms(500);
    }
    // 右侧空间更大
    else if (Right_Distance > Left_Distance)
    {
        Move_Right(speed); // 右转规避
        Delay_ms(500);
    }
    // 左侧空间更大
    else
    {
        Move_Left(speed); // 左转规避
        Delay_ms(500);
    }
}

/********************* 扫码模块处理函数 ?*********************/
void Scaner_Get(void)
{
    if (Serial_RxFlag1)  // 检测到完整数据包
    {
//        // 保存扫码数据
//        strncpy(g_scanData, (const char *)Serial_RxPacket1, 15-1);
//        g_scanData[15-1] = '\0'; // 确保字符串终止

        // 通过UART0转发扫码数据
        UART0_SendString("\r\n You are in:\r\n");
        UART0_SendString(Serial_RxPacket1);

        // OLED显示接收内容（第4行）
        OLED_ShowString(4, 1, "Sr:");
        OLED_ShowString(4, 4, (char *)Serial_RxPacket1);

        Serial_RxFlag1 = 0; // 清除接收标志
    }
}

void Scaner_stop(void)
{
    // 双保险：比较前强制补终止符
    g_Bluetooth[sizeof(g_Bluetooth)-1] = '\0';
    g_scanData[sizeof(g_scanData)-1] = '\0';

    if(strcmp(g_Bluetooth, g_scanData) == 0)
    {
        Car_Stop();
        UART0_SendString("EMG STOP!\r\n");
    }
}

// 在文件末尾添加中断服务程序
void BFTM1_IRQHandler(void)
{
    static uint16_t Count = 0; // 中断次数计数器
    
    Count++;
    if (Count >= 10) // 每10次中断执行一次PID计算
    {
        Count = 0;
        Motor_UpdateSpeed(); // 直接调用Motor模块的PID更新函数
    }
    BFTM_ClearFlag(HT_BFTM1);
}
