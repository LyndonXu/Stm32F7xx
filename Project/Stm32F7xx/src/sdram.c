#include <stdint.h>
#include "stm32f7xx_hal.h"

#include "sdram.h"

//SDRAM���ò���
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


static SDRAM_HandleTypeDef s_stSDRAMHandler;   //SDRAM���

static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *pHandle);
static int32_t SDRAM_Send_Cmd(uint32_t u32Bank, uint32_t u32Cmd, 
	uint32_t u32RefreshCount, uint32_t u32RegVal);

void SDRAM_Init(void)
{
	FMC_SDRAM_TimingTypeDef SDRAM_Timing;

	s_stSDRAMHandler.Instance = FMC_SDRAM_DEVICE;                                //SDRAM��BANK5,6
	s_stSDRAMHandler.Init.SDBank = FMC_SDRAM_BANK1;                         //SDRAM����BANK5��
	s_stSDRAMHandler.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;   //������
	s_stSDRAMHandler.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;        //������
	s_stSDRAMHandler.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;     //���ݿ��Ϊ16λ
	s_stSDRAMHandler.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4; //һ��4��BANK
	s_stSDRAMHandler.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;             //CASΪ3
	s_stSDRAMHandler.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE; //ʧ��д����
	s_stSDRAMHandler.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;         //SDRAMʱ��ΪHCLK/2=216M/2=108M=9.3ns
	s_stSDRAMHandler.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;              //ʹ��ͻ��
	s_stSDRAMHandler.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;          //��ͨ����ʱ

	SDRAM_Timing.LoadToActiveDelay = 2;                                 //����ģʽ�Ĵ���������ʱ����ӳ�Ϊ2��ʱ������
	SDRAM_Timing.ExitSelfRefreshDelay = 8;                              //�˳���ˢ���ӳ�Ϊ8��ʱ������
	SDRAM_Timing.SelfRefreshTime = 6;                                   //��ˢ��ʱ��Ϊ6��ʱ������
	SDRAM_Timing.RowCycleDelay = 6;                                     //��ѭ���ӳ�Ϊ6��ʱ������
	SDRAM_Timing.WriteRecoveryTime = 2;                                 //�ָ��ӳ�Ϊ2��ʱ������
	SDRAM_Timing.RPDelay = 2;                                           //��Ԥ����ӳ�Ϊ2��ʱ������
	SDRAM_Timing.RCDDelay = 2;                                          //�е����ӳ�Ϊ2��ʱ������
	HAL_SDRAM_Init(&s_stSDRAMHandler, &SDRAM_Timing);

	SDRAM_Initialization_Sequence(&s_stSDRAMHandler);  //����SDRAM��ʼ������

}

//����SDRAM��ʼ������
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *pHandle)
{
	uint32_t temp = 0;
	//SDRAM��������ʼ������Ժ���Ҫ��������˳���ʼ��SDRAM
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_CLK_ENABLE, 1, 0);  //ʱ������ʹ��
	
	HAL_Delay(2);									//������ʱ200us
	
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_PALL, 1, 0);    //�����д洢��Ԥ���
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);  //������ˢ�´���
	//����ģʽ�Ĵ���,SDRAM��bit0~bit2Ϊָ��ͻ�����ʵĳ��ȣ�
	//bit3Ϊָ��ͻ�����ʵ����ͣ�bit4~bit6ΪCASֵ��bit7��bit8Ϊ����ģʽ
	//bit9Ϊָ����дͻ��ģʽ��bit10��bit11λ����λ
	temp = (uint32_t) SDRAM_MODEREG_BURST_LENGTH_1          |	//����ͻ������:1(������1/2/4/8)
	       SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |	//����ͻ������:����(����������/����)
	       SDRAM_MODEREG_CAS_LATENCY_3           |	//����CASֵ:3(������2/3)
	       SDRAM_MODEREG_OPERATING_MODE_STANDARD |   //���ò���ģʽ:0,��׼ģʽ
	       SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;     //����ͻ��дģʽ:1,�������
	SDRAM_Send_Cmd(0, FMC_SDRAM_CMD_LOAD_MODE, 1, temp);  //����SDRAM��ģʽ�Ĵ���

	//ˢ��Ƶ�ʼ�����(��SDCLKƵ�ʼ���),���㷽��:
	//COUNT=SDRAMˢ������/����-20=SDRAMˢ������(us)*SDCLKƵ��(Mhz)/����
	//����ʹ�õ�SDRAMˢ������Ϊ64ms,SDCLK=216/2=108Mhz,����Ϊ8192(2^13).
	//����,COUNT=64*1000*108/8192-20=823
	HAL_SDRAM_ProgramRefreshRate(pHandle, 823);

}
//SDRAM�ײ��������������ã�ʱ��ʹ��
//�˺����ᱻHAL_SDRAM_Init()����
//hsdram:SDRAM���
void HAL_SDRAM_MspInit(SDRAM_HandleTypeDef *pHandle)
{
	GPIO_InitTypeDef GPIO_Initure;

	__HAL_RCC_FMC_CLK_ENABLE();                 //ʹ��FMCʱ��
	__HAL_RCC_GPIOC_CLK_ENABLE();               //ʹ��GPIOCʱ��
	__HAL_RCC_GPIOD_CLK_ENABLE();               //ʹ��GPIODʱ��
	__HAL_RCC_GPIOE_CLK_ENABLE();               //ʹ��GPIOEʱ��
	__HAL_RCC_GPIOF_CLK_ENABLE();               //ʹ��GPIOFʱ��
	__HAL_RCC_GPIOG_CLK_ENABLE();               //ʹ��GPIOGʱ��

	//��ʼ��PC0,2,3
	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_3;
	GPIO_Initure.Mode = GPIO_MODE_AF_PP;        //���츴��
	GPIO_Initure.Pull = GPIO_PULLUP;            //����
	GPIO_Initure.Speed = GPIO_SPEED_HIGH;       //����
	GPIO_Initure.Alternate = GPIO_AF12_FMC;     //����ΪFMC
	HAL_GPIO_Init(GPIOC, &GPIO_Initure);        //��ʼ��


	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &GPIO_Initure);  //��ʼ��PD0,1,8,9,10,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &GPIO_Initure);  //��ʼ��PE0,1,7,8,9,10,11,12,13,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOF, &GPIO_Initure);  //��ʼ��PF0,1,2,3,4,5,11,12,13,14,15

	GPIO_Initure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_8 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOG, &GPIO_Initure);	//��ʼ��PG0,1,2,4,5,8,15
}

//��SDRAM��������
//u32Bank:0,��BANK5�����SDRAM����ָ��
//      1,��BANK6�����SDRAM����ָ��
//u32Cmd:ָ��(0,����ģʽ/1,ʱ������ʹ��/2,Ԥ������д洢��/3,�Զ�ˢ��/4,����ģʽ�Ĵ���/5,��ˢ��/6,����)
//u32RefreshCount:��ˢ�´���
//u32RegVal:ģʽ�Ĵ����Ķ���
//����ֵ:0,����;-1,ʧ��.
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
	Command.CommandMode = u32Cmd;              //����
	Command.CommandTarget = u32TargetBank;    //Ŀ��SDRAM�洢����
	Command.AutoRefreshNumber = u32RefreshCount;    //��ˢ�´���
	Command.ModeRegisterDefinition = u32RegVal; //Ҫд��ģʽ�Ĵ�����ֵ
	if(HAL_SDRAM_SendCommand(&s_stSDRAMHandler, &Command, 0x1000) == HAL_OK)   //��SDRAM��������
	{
		return 0;
	}
	
	return -1;
}

#if 0
//��ָ����ַ(WriteAddr+Bank5_SDRAM_ADDR)��ʼ,����д��n���ֽ�.
//pBuffer:�ֽ�ָ��
//WriteAddr:Ҫд��ĵ�ַ
//n:Ҫд����ֽ���
void FMC_SDRAM_WriteBuffer(u8 *pBuffer, u32 WriteAddr, u32 n)
{
	for(; n != 0; n--)
	{
		* (vu8*)(Bank5_SDRAM_ADDR + WriteAddr) = *pBuffer;
		WriteAddr++;
		pBuffer++;
	}
}

//��ָ����ַ((WriteAddr+Bank5_SDRAM_ADDR))��ʼ,��������n���ֽ�.
//pBuffer:�ֽ�ָ��
//ReadAddr:Ҫ��������ʼ��ַ
//n:Ҫд����ֽ���
void FMC_SDRAM_ReadBuffer(u8 *pBuffer, u32 ReadAddr, u32 n)
{
	for(; n != 0; n--)
	{
		*pBuffer++ = * (vu8*)(Bank5_SDRAM_ADDR + ReadAddr);
		ReadAddr++;
	}
}
#endif

