#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "sdram.h"

static LTDC_HandleTypeDef  s_stLTDCHandler;	    //LTDC句柄
static DMA2D_HandleTypeDef s_stDMA2DHandler; 	    //DMA2D句柄

#define LCD_PIXFORMAT	LTDC_PIXEL_FORMAT_RGB565

#define HSW				1				    //水平同步宽度
#define VSW				1				    //垂直同步宽度
#define HBP				40				    //水平后廊
#define VBP				8				    //垂直后廊
#define HFP				5				    //水平前廊
#define VFP				8				    //垂直前廊

#define LCD_WIDTH		480
#define LCD_HEIGHT		272


#define LCD_LED(on) (on ? HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_SET) : HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET))


static uint16_t s_u16LTDCFrameBuf[2][1280][800] __attribute__((at(SDRAM_ADDR)));

static const uint32_t s_c_u32LTDCFrameBufAddr[2] = 
{ 
	//(uint32_t)(s_u16LTDCFrameBuf), 
	//(uint32_t)(s_u16LTDCFrameBuf) + sizeof(s_u16LTDCFrameBuf) / 2,
	SDRAM_ADDR,
	SDRAM_ADDR + sizeof(s_u16LTDCFrameBuf) / 2,
};


//LTDC底层IO初始化和时钟使能
//此函数会被HAL_LTDC_Init()调用
//hltdc:LTDC句柄
void HAL_LTDC_MspInit(LTDC_HandleTypeDef* hltdc)
{
	GPIO_InitTypeDef GPIO_Initure;

	__HAL_RCC_LTDC_CLK_ENABLE();                //使能LTDC时钟
	__HAL_RCC_DMA2D_CLK_ENABLE();               //使能DMA2D时钟
	__HAL_RCC_GPIOB_CLK_ENABLE();               //使能GPIOB时钟
	__HAL_RCC_GPIOF_CLK_ENABLE();               //使能GPIOF时钟
	__HAL_RCC_GPIOG_CLK_ENABLE();               //使能GPIOG时钟
	__HAL_RCC_GPIOH_CLK_ENABLE();               //使能GPIOH时钟
	__HAL_RCC_GPIOI_CLK_ENABLE();               //使能GPIOI时钟

	//初始化PB5，背光引脚
	GPIO_Initure.Pin = GPIO_PIN_5;              //PB5推挽输出，控制背光
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;    //推挽输出
	GPIO_Initure.Pull = GPIO_PULLUP;            //上拉
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //高速
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);

	//初始化PF10
	GPIO_Initure.Pin = GPIO_PIN_10;
	GPIO_Initure.Mode = GPIO_MODE_AF_PP;        //复用
	GPIO_Initure.Pull = GPIO_NOPULL;
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //高速
	GPIO_Initure.Alternate = GPIO_AF14_LTDC;    //复用为LTDC
	HAL_GPIO_Init(GPIOF, &GPIO_Initure);

	//初始化PG6,7,11
	GPIO_Initure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_11;
	HAL_GPIO_Init(GPIOG, &GPIO_Initure);

	//初始化PH9,10,11,12,13,14,15
	GPIO_Initure.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
	                   GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOH, &GPIO_Initure);

	//初始化PI0,1,2,4,5,6,7,9,10
	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | \
	                   GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_9 | GPIO_PIN_10;
	HAL_GPIO_Init(GPIOI, &GPIO_Initure);
}

void LTDC_Layer_Parameter_Config(uint8_t u8LayerIndex,
                                 uint32_t u32BufAddr,
                                 uint8_t u8PixFormat,
                                 uint8_t u8Alpha,
                                 uint8_t u8Alpha0,
                                 uint32_t u32BlendingFactor1,
                                 uint32_t u32BlendingFactor2,
                                 uint32_t u32BackColor)
{
	LTDC_LayerCfgTypeDef stLayerCfg;

	stLayerCfg.WindowX0 = 0;                     //窗口起始X坐标
	stLayerCfg.WindowY0 = 0;                     //窗口起始Y坐标
	stLayerCfg.WindowX1 = LCD_WIDTH;        	//窗口终止X坐标
	stLayerCfg.WindowY1 = LCD_HEIGHT;       	//窗口终止Y坐标
	stLayerCfg.PixelFormat = u8PixFormat;		  //像素格式
	stLayerCfg.Alpha = u8Alpha;				      //Alpha值设置，0~255,255为完全不透明
	stLayerCfg.Alpha0 = u8Alpha0;			      //默认Alpha值
	stLayerCfg.BlendingFactor1 = u32BlendingFactor1; //设置层混合系数
	stLayerCfg.BlendingFactor2 = u32BlendingFactor2;	//设置层混合系数
	stLayerCfg.FBStartAdress = u32BufAddr;	      //设置层颜色帧缓存起始地址
	stLayerCfg.ImageWidth = LCD_WIDTH;      //设置颜色帧缓冲区的宽度
	stLayerCfg.ImageHeight = LCD_HEIGHT;    //设置颜色帧缓冲区的高度
	stLayerCfg.Backcolor.Red = (uint8_t)(u32BackColor & 0X00FF0000) >> 16; //背景颜色红色部分
	stLayerCfg.Backcolor.Green = (uint8_t)(u32BackColor & 0X0000FF00) >> 8; //背景颜色绿色部分
	stLayerCfg.Backcolor.Blue = (uint8_t)u32BackColor & 0X000000FF;    //背景颜色蓝色部分
	HAL_LTDC_ConfigLayer(&s_stLTDCHandler, &stLayerCfg, u8LayerIndex);  //设置所选中的层
}


void LTDC_Layer_Window_Config(uint8_t u8LayerIndex, uint16_t x, uint16_t y,
                              uint16_t u16Width, uint16_t u16Height)
{
	HAL_LTDC_SetWindowPosition(&s_stLTDCHandler, x, y, u8LayerIndex); //设置窗口的位置
	HAL_LTDC_SetWindowSize(&s_stLTDCHandler, u16Width, u16Height, u8LayerIndex);//设置窗口大小
}


void LTDC_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t u32Color)
{
	uint32_t u32Timeout = 0;
	uint16_t u16OffLine;
	uint32_t u32Addr;

	u16OffLine = LCD_WIDTH - (ex - sx + 1);
	u32Addr = s_c_u32LTDCFrameBufAddr[0] + 2 * (LCD_WIDTH * sy + sx);
	//RCC->AHB1ENR |= 1 << 23;			//使能DM2D时钟
	
	DMA2D->CR = DMA2D_R2M;				//寄存器到存储器模式
	DMA2D->OPFCCR = LCD_PIXFORMAT;	//设置颜色格式
	DMA2D->OOR = u16OffLine;				//设置行偏移
	DMA2D->CR &= ~(DMA2D_CR_START);				//先停止DMA2D
	DMA2D->OMAR = u32Addr;				//输出存储器地址
	DMA2D->NLR = (ey - sy + 1) | ((ex - sx + 1) << 16);	//设定行数寄存器
	DMA2D->OCOLR = u32Color;				//设定输出颜色寄存器
	DMA2D->CR |= DMA2D_CR_START;				//启动DMA2D
	
	while((DMA2D->ISR & (DMA2D_FLAG_TC)) == 0)	//等待传输完成
	{
		u32Timeout++;
		if (u32Timeout > 0x1FFFFF)
			break;	//超时退出
	}
	DMA2D->IFCR |= DMA2D_FLAG_TC;				//清除传输完成标志
}

void LTDC_Clear(uint32_t u32olor)
{
	LTDC_Fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, u32olor);
}

int32_t LTDC_Init(void)
{

	do
	{
		RCC_PeriphCLKInitTypeDef PeriphClkIniture;

		//LTDC输出像素时钟，需要根据自己所使用的LCD数据手册来配置！
		PeriphClkIniture.PeriphClockSelection = RCC_PERIPHCLK_LTDC;	//LTDC时钟
		PeriphClkIniture.PLLSAI.PLLSAIN = 288;
		PeriphClkIniture.PLLSAI.PLLSAIR = 4;
		PeriphClkIniture.PLLSAIDivR = RCC_PLLSAIDIVR_8;	/* > 480 * 272 * 60fps / 1024 / 1024 */
		if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkIniture) != HAL_OK) //配置像素时钟，这里配置为时钟为18.75MHZ
		{
			return -1;   //成功
		}
	}
	while(0);


	s_stLTDCHandler.Instance = LTDC;
	s_stLTDCHandler.Init.HSPolarity = LTDC_HSPOLARITY_AL;       //水平同步极性
	s_stLTDCHandler.Init.VSPolarity = LTDC_VSPOLARITY_AL;       //垂直同步极性
	s_stLTDCHandler.Init.DEPolarity = LTDC_DEPOLARITY_AL;       //数据使能极性
	s_stLTDCHandler.Init.PCPolarity = LTDC_PCPOLARITY_IPC;      //像素时钟极性
	s_stLTDCHandler.Init.HorizontalSync = HSW - 1;      //水平同步宽度
	s_stLTDCHandler.Init.VerticalSync = VSW - 1;        //垂直同步宽度
	s_stLTDCHandler.Init.AccumulatedHBP = HSW + HBP - 1; //水平同步后沿宽度
	s_stLTDCHandler.Init.AccumulatedVBP = VSW + VBP - 1; //垂直同步后沿高度
	s_stLTDCHandler.Init.AccumulatedActiveW = HSW + HBP + LCD_WIDTH - 1; //有效宽度
	s_stLTDCHandler.Init.AccumulatedActiveH = VSW + VBP + LCD_HEIGHT - 1; //有效高度
	s_stLTDCHandler.Init.TotalWidth = HSW + HBP + LCD_WIDTH + HFP - 1; //总宽度
	s_stLTDCHandler.Init.TotalHeigh = VSW + VBP + LCD_HEIGHT + VFP - 1; //总高度
	s_stLTDCHandler.Init.Backcolor.Red = 0;         //屏幕背景层红色部分
	s_stLTDCHandler.Init.Backcolor.Green = 0;       //屏幕背景层绿色部分
	s_stLTDCHandler.Init.Backcolor.Blue = 0;        //屏幕背景色蓝色部分
	HAL_LTDC_Init(&s_stLTDCHandler);

	//层配置
	LTDC_Layer_Parameter_Config(0, s_c_u32LTDCFrameBufAddr[0],
	                            LCD_PIXFORMAT, 255, 0, 
								LTDC_BLENDING_FACTOR1_PAxCA, 
								LTDC_BLENDING_FACTOR2_PAxCA, 0x000000); //层参数配置
	LTDC_Layer_Window_Config(0, 0, 0, LCD_WIDTH, LCD_HEIGHT);	//层窗口配置,以LCD面板坐标系为基准,不要随便修改!


	LCD_LED(1);         		    //点亮背光
	LTDC_Clear(0xFFFFFFFF);			//清屏

	return 0;
}

