#include <stdint.h>
#include "stm32f7xx_hal.h"

#include "sdram.h"

//SDRAM配置参数
#define SDRAM_MODEREG_BURST_LENGTH_1             (0x0000)
#define SDRAM_MODEREG_BURST_LENGTH_2             (0x0001)
#define SDRAM_MODEREG_BURST_LENGTH_4             (0x0002)
#define SDRAM_MODEREG_BURST_LENGTH_8             (0x0004)
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      (0x0000)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     (0x0008)
#define SDRAM_MODEREG_CAS_LATENCY_2              (0x0020)
#define SDRAM_MODEREG_CAS_LATENCY_3              (0x0030)
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    (0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED (0x0000)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     (0x0200)


static SDRAM_HandleTypeDef s_stSDRAMHandler;   //SDRAM句柄

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *pHandle);
static int32_t SDRAM_Send_Cmd(uint32_t u32Bank, uint32_t u32Cmd, 
	uint32_t u32RefreshCount, uint32_t u32RegVal);

void SDRAM_Init(void)
{
	FMC_SDRAM_TimingTypeDef SDRAM_Timing;

	s_stSDRAMHandler.Instance = FMC_SDRAM_DEVICE;                                //SDRAM在BANK5,6
	s_stSDRAMHandler.Init.SDBank = FMC_SDRAM_BANK1;                         //SDRAM接在BANK5上
	s_stSDRAMHandler.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;   //列数量
	s_stSDRAMHandler.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;        //行数量
	s_stSDRAMHandler.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;     //数据宽度为16位
	s_stSDRAMHandler.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4; //一共4个BANK
	s_stSDRAMHandler.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;             //CAS为3
	s_stSDRAMHandler.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE; //失能写保护
	s_stSDRAMHandler.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;         //SDRAM时钟为HCLK/2=216M/2=108M=9.3ns
	s_stSDRAMHandler.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;              //使能突发
	s_stSDRAMHandler.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;          //读通道延时

	SDRAM_Timing.LoadToActiveDelay = 2;                                 //加载模式寄存器到激活时间的延迟为2个时钟周期
	SDRAM_Timing.ExitSelfRefreshDelay = 8;                              //退出自刷新延迟为8个时钟周期
	SDRAM_Timing.SelfRefreshTime = 6;                                   //自刷新时间为6个时钟周期
	SDRAM_Timing.RowCycleDelay = 6;                                     //行循环延迟为6个时钟周期
	SDRAM_Timing.WriteRecoveryTime = 2;                                 //恢复延迟为2个时钟周期
	SDRAM_Timing.RPDelay = 2;                                           //行预充电延迟为2个时钟周期
	SDRAM_Timing.RCDDelay = 2;                                          //行到列延迟为2个时钟周期
	HAL_SDRAM_Init(&s_stSDRAMHandler, &SDRAM_Timing);

	SDRAM_Initialization_Sequence(&s_stSDRAMHandler);  //发送SDRAM初始化序列

}

//发送SDRAM初始化序列
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *pHandle)
{
	uint32_t temp = 0;
	//SDRAM控制器初始化完成以后还需要按照如下顺序初始化SDRAM
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_CLK_ENABLE, 1, 0);  //时钟配置使能
	
	HAL_Delay(2);									//至少延时200us
	
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_PALL, 1, 0);    //对所有存储区预充电
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);  //设置自刷新次数
	//配置模式寄存器,SDRAM的bit0~bit2为指定突发访问的长度，
	//bit3为指定突发访问的类型，bit4~bit6为CAS值，bit7和bit8为运行模式
	//bit9为指定的写突发模式，bit10和bit11位保留位
	temp = (uint32_t) SDRAM_MODEREG_BURST_LENGTH_1          |	//设置突发长度:1(可以是1/2/4/8)
	       SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |	//设置突发类型:连续(可以是连续/交错)
	       SDRAM_MODEREG_CAS_LATENCY_3           |	//设置CAS值:3(可以是2/3)
	       SDRAM_MODEREG_OPERATING_MODE_STANDARD |   //设置操作模式:0,标准模式
	       SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;     //设置突发写模式:1,单点访问
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_LOAD_MODE, 1, temp);  //设置SDRAM的模式寄存器

	//刷新频率计数器(以SDCLK频率计数),计算方法:
	//COUNT=SDRAM刷新周期/行数-20=SDRAM刷新周期(us)*SDCLK频率(Mhz)/行数
	//我们使用的SDRAM刷新周期为64ms,SDCLK=216/2=108Mhz,行数为8192(2^13).
	//所以,COUNT=64*1000*108/8192-20=823
	HAL_SDRAM_ProgramRefreshRate(pHandle, 823);

}
//SDRAM底层驱动，引脚配置，时钟使能
//此函数会被HAL_SDRAM_Init()调用
//hsdram:SDRAM句柄
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *pHandle)
{
	GPIO_InitTypeDef GPIO_Initure;

	__HAL_RCC_FMC_CLK_ENABLE();                 //使能FMC时钟
	__HAL_RCC_GPIOC_CLK_ENABLE();               //使能GPIOC时钟
	__HAL_RCC_GPIOD_CLK_ENABLE();               //使能GPIOD时钟
	__HAL_RCC_GPIOE_CLK_ENABLE();               //使能GPIOE时钟
	__HAL_RCC_GPIOF_CLK_ENABLE();               //使能GPIOF时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();               //使能GPIOG时钟

	//初始化PC0,2,3
	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_Initure.Mode = GPIO_MODE_AF_PP;        //推挽复用
	GPIO_Initure.Pull = GPIO_PULLUP;            //上拉
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //高速
	GPIO_Initure.Alternate = GPIO_AF12_FMC;     //复用为FMC
	HAL_GPIO_Init(GPIOC, &GPIO_Initure);        //初始化


	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &GPIO_Initure);  //初始化PD0,1,8,9,10,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &GPIO_Initure);  //初始化PE0,1,7,8,9,10,11,12,13,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOF, &GPIO_Initure);  //初始化PF0,1,2,3,4,5,11,12,13,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOG, &GPIO_Initure);	//初始化PG0,1,2,4,5,8,15
}

//向SDRAM发送命令
//u32Bank:0,向BANK5上面的SDRAM发送指令
//      1,向BANK6上面的SDRAM发送指令
//u32Cmd:指令(0,正常模式/1,时钟配置使能/2,预充电所有存储区/3,自动刷新/4,加载模式寄存器/5,自刷新/6,掉电)
//u32RefreshCount:自刷新次数
//u32RegVal:模式寄存器的定义
//返回值:0,正常;-1,失败.
static int32_t SDRAM_Send_Cmd(uint32_t u32Bank, uint32_t u32Cmd, 
	uint32_t u32RefreshCount, uint32_t u32RegVal)
{
	uint32_t u32TargetBank = 0;
	FMC_SDRAM_CommandTypeDef Command;

	if(u32Bank == 0) 
	{
		u32TargetBank = FMC_SDRAM_CMD_TARGET_BANK1;
	}
	else if(u32Bank == 1) 
	{
		u32TargetBank = FMC_SDRAM_CMD_TARGET_BANK2;
	}
	Command.CommandMode = u32Cmd;              //命令
	Command.CommandTarget = u32TargetBank;    //目标SDRAM存储区域
	Command.AutoRefreshNumber = u32RefreshCount;    //自刷新次数
	Command.ModeRegisterDefinition = u32RegVal; //要写入模式寄存器的值
	if(HAL_SDRAM_SendCommand(&s_stSDRAMHandler, &Command, 0x1000) == HAL_OK)   //向SDRAM发送命令
	{
		return 0;
	}
	
	return -1;
}

#if 0
//在指定地址(WriteAddr+Bank5_SDRAM_ADDR)开始,连续写入n个字节.
//pBuffer:字节指针
//WriteAddr:要写入的地址
//n:要写入的字节数
void FMC_SDRAM_WriteBuffer(u8 *pBuffer, u32 WriteAddr, u32 n)
{
	for(; n != 0; n--)
	{
		* (vu8*)(Bank5_SDRAM_ADDR + WriteAddr) = *pBuffer;
		WriteAddr++;
		pBuffer++;
	}
}

//在指定地址((WriteAddr+Bank5_SDRAM_ADDR))开始,连续读出n个字节.
//pBuffer:字节指针
//ReadAddr:要读出的起始地址
//n:要写入的字节数
void FMC_SDRAM_ReadBuffer(u8 *pBuffer, u32 ReadAddr, u32 n)
{
	for(; n != 0; n--)
	{
		*pBuffer++ = * (vu8*)(Bank5_SDRAM_ADDR + ReadAddr);
		ReadAddr++;
	}
}
#endif

