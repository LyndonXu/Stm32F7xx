#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "sdram.h"

static LTDC_HandleTypeDef  s_stLTDCHandler;	    //LTDC���
static DMA2D_HandleTypeDef s_stDMA2DHandler; 	    //DMA2D���

#define LCD_PIXFORMAT	LTDC_PIXEL_FORMAT_RGB565

#define HSW				1				    //ˮƽͬ�����
#define VSW				1				    //��ֱͬ�����
#define HBP				40				    //ˮƽ����
#define VBP				8				    //��ֱ����
#define HFP				5				    //ˮƽǰ��
#define VFP				8				    //��ֱǰ��

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


//LTDC�ײ�IO��ʼ����ʱ��ʹ��
//�˺����ᱻHAL_LTDC_Init()����
//hltdc:LTDC���
void HAL_LTDC_MspInit(LTDC_HandleTypeDef* hltdc)
{
	GPIO_InitTypeDef GPIO_Initure;

	__HAL_RCC_LTDC_CLK_ENABLE();                //ʹ��LTDCʱ��
	__HAL_RCC_DMA2D_CLK_ENABLE();               //ʹ��DMA2Dʱ��
	__HAL_RCC_GPIOB_CLK_ENABLE();               //ʹ��GPIOBʱ��
	__HAL_RCC_GPIOF_CLK_ENABLE();               //ʹ��GPIOFʱ��
	__HAL_RCC_GPIOG_CLK_ENABLE();               //ʹ��GPIOGʱ��
	__HAL_RCC_GPIOH_CLK_ENABLE();               //ʹ��GPIOHʱ��
	__HAL_RCC_GPIOI_CLK_ENABLE();               //ʹ��GPIOIʱ��

	//��ʼ��PB5����������
	GPIO_Initure.Pin = GPIO_PIN_5;              //PB5������������Ʊ���
	GPIO_Initure.Mode = GPIO_MODE_OUTPUT_PP;    //�������
	GPIO_Initure.Pull = GPIO_PULLUP;            //����
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //����
	HAL_GPIO_Init(GPIOB, &GPIO_Initure);

	//��ʼ��PF10
	GPIO_Initure.Pin = GPIO_PIN_10;
	GPIO_Initure.Mode = GPIO_MODE_AF_PP;        //����
	GPIO_Initure.Pull = GPIO_NOPULL;
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //����
	GPIO_Initure.Alternate = GPIO_AF14_LTDC;    //����ΪLTDC
	HAL_GPIO_Init(GPIOF, &GPIO_Initure);

	//��ʼ��PG6,7,11
	GPIO_Initure.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_11;
	HAL_GPIO_Init(GPIOG, &GPIO_Initure);

	//��ʼ��PH9,10,11,12,13,14,15
	GPIO_Initure.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | \
	                   GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOH, &GPIO_Initure);

	//��ʼ��PI0,1,2,4,5,6,7,9,10
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

	stLayerCfg.WindowX0 = 0;                     //������ʼX����
	stLayerCfg.WindowY0 = 0;                     //������ʼY����
	stLayerCfg.WindowX1 = LCD_WIDTH;        	//������ֹX����
	stLayerCfg.WindowY1 = LCD_HEIGHT;       	//������ֹY����
	stLayerCfg.PixelFormat = u8PixFormat;		  //���ظ�ʽ
	stLayerCfg.Alpha = u8Alpha;				      //Alphaֵ���ã�0~255,255Ϊ��ȫ��͸��
	stLayerCfg.Alpha0 = u8Alpha0;			      //Ĭ��Alphaֵ
	stLayerCfg.BlendingFactor1 = u32BlendingFactor1; //���ò���ϵ��
	stLayerCfg.BlendingFactor2 = u32BlendingFactor2;	//���ò���ϵ��
	stLayerCfg.FBStartAdress = u32BufAddr;	      //���ò���ɫ֡������ʼ��ַ
	stLayerCfg.ImageWidth = LCD_WIDTH;      //������ɫ֡�������Ŀ��
	stLayerCfg.ImageHeight = LCD_HEIGHT;    //������ɫ֡�������ĸ߶�
	stLayerCfg.Backcolor.Red = (uint8_t)(u32BackColor & 0X00FF0000) >> 16; //������ɫ��ɫ����
	stLayerCfg.Backcolor.Green = (uint8_t)(u32BackColor & 0X0000FF00) >> 8; //������ɫ��ɫ����
	stLayerCfg.Backcolor.Blue = (uint8_t)u32BackColor & 0X000000FF;    //������ɫ��ɫ����
	HAL_LTDC_ConfigLayer(&s_stLTDCHandler, &stLayerCfg, u8LayerIndex);  //������ѡ�еĲ�
}


void LTDC_Layer_Window_Config(uint8_t u8LayerIndex, uint16_t x, uint16_t y,
                              uint16_t u16Width, uint16_t u16Height)
{
	HAL_LTDC_SetWindowPosition(&s_stLTDCHandler, x, y, u8LayerIndex); //���ô��ڵ�λ��
	HAL_LTDC_SetWindowSize(&s_stLTDCHandler, u16Width, u16Height, u8LayerIndex);//���ô��ڴ�С
}


void LTDC_Fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t u32Color)
{
	uint32_t u32Timeout = 0;
	uint16_t u16OffLine;
	uint32_t u32Addr;

	u16OffLine = LCD_WIDTH - (ex - sx + 1);
	u32Addr = s_c_u32LTDCFrameBufAddr[0] + 2 * (LCD_WIDTH * sy + sx);
	//RCC->AHB1ENR |= 1 << 23;			//ʹ��DM2Dʱ��
	
	DMA2D->CR = DMA2D_R2M;				//�Ĵ������洢��ģʽ
	DMA2D->OPFCCR = LCD_PIXFORMAT;	//������ɫ��ʽ
	DMA2D->OOR = u16OffLine;				//������ƫ��
	DMA2D->CR &= ~(DMA2D_CR_START);				//��ֹͣDMA2D
	DMA2D->OMAR = u32Addr;				//����洢����ַ
	DMA2D->NLR = (ey - sy + 1) | ((ex - sx + 1) << 16);	//�趨�����Ĵ���
	DMA2D->OCOLR = u32Color;				//�趨�����ɫ�Ĵ���
	DMA2D->CR |= DMA2D_CR_START;				//����DMA2D
	
	while((DMA2D->ISR & (DMA2D_FLAG_TC)) == 0)	//�ȴ��������
	{
		u32Timeout++;
		if (u32Timeout > 0x1FFFFF)
			break;	//��ʱ�˳�
	}
	DMA2D->IFCR |= DMA2D_FLAG_TC;				//���������ɱ�־
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

		//LTDC�������ʱ�ӣ���Ҫ�����Լ���ʹ�õ�LCD�����ֲ������ã�
		PeriphClkIniture.PeriphClockSelection = RCC_PERIPHCLK_LTDC;	//LTDCʱ��
		PeriphClkIniture.PLLSAI.PLLSAIN = 288;
		PeriphClkIniture.PLLSAI.PLLSAIR = 4;
		PeriphClkIniture.PLLSAIDivR = RCC_PLLSAIDIVR_8;	/* > 480 * 272 * 60fps / 1024 / 1024 */
		if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkIniture) != HAL_OK) //��������ʱ�ӣ���������Ϊʱ��Ϊ18.75MHZ
		{
			return -1;   //�ɹ�
		}
	}
	while(0);


	s_stLTDCHandler.Instance = LTDC;
	s_stLTDCHandler.Init.HSPolarity = LTDC_HSPOLARITY_AL;       //ˮƽͬ������
	s_stLTDCHandler.Init.VSPolarity = LTDC_VSPOLARITY_AL;       //��ֱͬ������
	s_stLTDCHandler.Init.DEPolarity = LTDC_DEPOLARITY_AL;       //����ʹ�ܼ���
	s_stLTDCHandler.Init.PCPolarity = LTDC_PCPOLARITY_IPC;      //����ʱ�Ӽ���
	s_stLTDCHandler.Init.HorizontalSync = HSW - 1;      //ˮƽͬ�����
	s_stLTDCHandler.Init.VerticalSync = VSW - 1;        //��ֱͬ�����
	s_stLTDCHandler.Init.AccumulatedHBP = HSW + HBP - 1; //ˮƽͬ�����ؿ��
	s_stLTDCHandler.Init.AccumulatedVBP = VSW + VBP - 1; //��ֱͬ�����ظ߶�
	s_stLTDCHandler.Init.AccumulatedActiveW = HSW + HBP + LCD_WIDTH - 1; //��Ч���
	s_stLTDCHandler.Init.AccumulatedActiveH = VSW + VBP + LCD_HEIGHT - 1; //��Ч�߶�
	s_stLTDCHandler.Init.TotalWidth = HSW + HBP + LCD_WIDTH + HFP - 1; //�ܿ��
	s_stLTDCHandler.Init.TotalHeigh = VSW + VBP + LCD_HEIGHT + VFP - 1; //�ܸ߶�
	s_stLTDCHandler.Init.Backcolor.Red = 0;         //��Ļ�������ɫ����
	s_stLTDCHandler.Init.Backcolor.Green = 0;       //��Ļ��������ɫ����
	s_stLTDCHandler.Init.Backcolor.Blue = 0;        //��Ļ����ɫ��ɫ����
	HAL_LTDC_Init(&s_stLTDCHandler);

	//������
	LTDC_Layer_Parameter_Config(0, s_c_u32LTDCFrameBufAddr[0],
	                            LCD_PIXFORMAT, 255, 0, 
								LTDC_BLENDING_FACTOR1_PAxCA, 
								LTDC_BLENDING_FACTOR2_PAxCA, 0x000000); //���������
	LTDC_Layer_Window_Config(0, 0, 0, LCD_WIDTH, LCD_HEIGHT);	//�㴰������,��LCD�������ϵΪ��׼,��Ҫ����޸�!


	LCD_LED(1);         		    //��������
	LTDC_Clear(0xFFFFFFFF);			//����

	return 0;
}

