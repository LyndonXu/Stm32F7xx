/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：message.c
* 摘要: 协议自动检测程序
* 版本：0.0.1
* 作者：许龙杰
* 日期：2013年01月25日
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "stm32f7xx_hal.h"

//#include "user_api.h"
#include "app_port.h"

#include "protocol.h"
#include "message.h"



#define MSG_RX_PIN 					GPIO_PIN_10
#define MSG_TX_PIN					GPIO_PIN_9
#define MSG_PORT					GPIOA
#define MSG_UART					USART1

#define MSG_RX_TX_AF				GPIO_AF7_USART1

#define ENABLE_MSG_UART()			__USART1_CLK_ENABLE()

#define MSG_UART_IRQ_CHANNEL		USART1_IRQn
#define MSG_UART_IRQ				USART1_IRQHandler

#define ENABLE_UART_DMA()			__DMA2_CLK_ENABLE()

/* Definition for USARTx's DMA */
#define MSG_TX_DMA_STREAM				DMA2_Stream7
#define MSG_RX_DMA_STREAM				DMA2_Stream5

#define MSG_TX_DMA_CHANNEL				DMA_CHANNEL_4
#define MSG_RX_DMA_CHANNEL				DMA_CHANNEL_4

#define MSG_DMA_TX_IRQn					DMA2_Stream7_IRQn
#define MSG_DMA_RX_IRQn					DMA2_Stream5_IRQn
#define MSG_DMA_TX_IRQHandler			DMA2_Stream7_IRQHandler
#define MSG_DMA_RX_IRQHandler			DMA2_Stream5_IRQHandler


#ifndef MAX_IO_FIFO_CNT
#define MAX_IO_FIFO_CNT 16
#endif

#ifndef LEVEL_ONE_CACHE_CNT
#define LEVEL_ONE_CACHE_CNT 64
#endif

#ifndef CYCLE_BUF_LENGTH
#define CYCLE_BUF_LENGTH		(512 * 2)
#endif


static StIOFIFOList s_stIOFIFOList[MAX_IO_FIFO_CNT];
static StIOFIFOCtrl s_stIOFIFOCtrl;

static char s_c8LevelOneCache[LEVEL_ONE_CACHE_CNT * 2];
static StLevelOneCache s_stLevelOneCache;

static char s_c8CycleBuf[CYCLE_BUF_LENGTH];
static StCycleBuf s_stCycleBuf;

static UART_HandleTypeDef s_stUart1Handle;
static DMA_HandleTypeDef s_stHDMAForUart1TX;



static int32_t UARTInit(void)
{

	/*##-1- Configure the UART peripheral ######################################*/
	/* Put the USART peripheral in the Asynchronous mode (UART Mode) */
	/* UART configured as follows:
	    - Word Length = 8 Bits (7 data bit + 1 parity bit) :
	                  BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
	    - Stop Bit    = One Stop bit
	    - Parity      = ODD parity
	    - BaudRate    = 9600 baud
	    - Hardware flow control disabled (RTS and CTS signals) */
	s_stUart1Handle.Instance          = MSG_UART;

	s_stUart1Handle.Init.BaudRate     = 115200;
	s_stUart1Handle.Init.WordLength   = UART_WORDLENGTH_8B;
	s_stUart1Handle.Init.StopBits     = UART_STOPBITS_1;
	s_stUart1Handle.Init.Parity       = UART_PARITY_NONE;
	s_stUart1Handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	s_stUart1Handle.Init.Mode         = UART_MODE_TX_RX;
	s_stUart1Handle.Init.OverSampling = UART_OVERSAMPLING_16;

	if(HAL_UART_Init(&s_stUart1Handle) != HAL_OK)
	{
		/* Initialization Error */
		return -1;
	}
#if 0	
	if (HAL_UART_Transmit_DMA(&s_stUart1Handle, (uint8_t *)"test ok\n", 9) != HAL_OK)
	{
		/* Transfer error in transmission process */
	}	
#endif
	return 0;

}

int32_t MessageUARTInitMsp(void)
{
	//static DMA_HandleTypeDef hdma_rx;

	GPIO_InitTypeDef  GPIO_InitStruct;

	RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO clock */
	__GPIOA_CLK_ENABLE();

	/* Select SysClk as source of USART1 clocks */
	RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
	RCC_PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_SYSCLK;
	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);


	/* Enable USARTx clock */
	__USART1_CLK_ENABLE();

	/* Enable DMA clock */
	__DMA2_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin       = MSG_TX_PIN;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_PULLUP;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = MSG_RX_TX_AF;

	HAL_GPIO_Init(MSG_PORT, &GPIO_InitStruct);

	/* UART RX GPIO pin configuration  */
	GPIO_InitStruct.Pin = MSG_RX_PIN;
	GPIO_InitStruct.Alternate = MSG_RX_TX_AF;

	HAL_GPIO_Init(MSG_PORT, &GPIO_InitStruct);

	/*##-3- Configure the DMA ##################################################*/
	/* Configure the DMA handler for Transmission process */
	s_stHDMAForUart1TX.Instance                 = MSG_TX_DMA_STREAM;
	s_stHDMAForUart1TX.Init.Channel             = MSG_TX_DMA_CHANNEL;
	s_stHDMAForUart1TX.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	s_stHDMAForUart1TX.Init.PeriphInc           = DMA_PINC_DISABLE;
	s_stHDMAForUart1TX.Init.MemInc              = DMA_MINC_ENABLE;
	s_stHDMAForUart1TX.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	s_stHDMAForUart1TX.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	s_stHDMAForUart1TX.Init.Mode                = DMA_NORMAL;
	s_stHDMAForUart1TX.Init.Priority            = DMA_PRIORITY_LOW;
	s_stHDMAForUart1TX.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
	s_stHDMAForUart1TX.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	s_stHDMAForUart1TX.Init.MemBurst            = DMA_MBURST_INC4;
	s_stHDMAForUart1TX.Init.PeriphBurst         = DMA_PBURST_INC4;

	HAL_DMA_Init(&s_stHDMAForUart1TX);

	/* Associate the initialized DMA handle to the UART handle */
	__HAL_LINKDMA(&s_stUart1Handle, hdmatx, s_stHDMAForUart1TX);
#if 0
	/* Configure the DMA handler for reception process */
	hdma_rx.Instance                 = MSG_RX_DMA_STREAM;
	hdma_rx.Init.Channel             = MSG_RX_DMA_CHANNEL;
	hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
	hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
	hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	hdma_rx.Init.Mode                = DMA_NORMAL;
	hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;
	hdma_rx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
	hdma_rx.Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
	hdma_rx.Init.MemBurst            = DMA_MBURST_INC4;
	hdma_rx.Init.PeriphBurst         = DMA_PBURST_INC4;

	HAL_DMA_Init(&hdma_rx);

	/* Associate the initialized DMA handle to the the UART handle */
	__HAL_LINKDMA(huart, hdmarx, hdma_rx);
#endif
	/*##-4- Configure the NVIC for DMA #########################################*/
	/* NVIC configuration for DMA transfer complete interrupt (USARTx_TX) */
	HAL_NVIC_SetPriority(MSG_DMA_TX_IRQn, 4, 1);
	HAL_NVIC_EnableIRQ(MSG_DMA_TX_IRQn);
#if 0
	/* NVIC configuration for DMA transfer complete interrupt (USARTx_RX) */
	HAL_NVIC_SetPriority(USARTx_DMA_RX_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(USARTx_DMA_RX_IRQn);
#endif
	/* NVIC configuration for USART, to catch the TX complete */
	__HAL_UART_ENABLE_IT(&s_stUart1Handle, UART_IT_RXNE);
	HAL_NVIC_SetPriority(MSG_UART_IRQ_CHANNEL, 4, 1);
	HAL_NVIC_EnableIRQ(MSG_UART_IRQ_CHANNEL);
	
	return 0;

}


/**
  * @brief  This function handles DMA TX interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to DMA stream
  *         used for USART data reception
  */
void MSG_DMA_TX_IRQHandler(void)
{
	HAL_DMA_IRQHandler(s_stUart1Handle.hdmatx);
}


/**
  * @brief  This function handles USARTx interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to DMA
  *         used for USART data transmission
  */
void MSG_UART_IRQ(void)
{
	if((__HAL_UART_GET_FLAG(&s_stUart1Handle, UART_FLAG_RXNE) != RESET))
	{
		uint8_t u8RxTmp = s_stUart1Handle.Instance->RDR;
		LOCWriteSomeData(&s_stLevelOneCache, &u8RxTmp, 1);
	}
	HAL_UART_IRQHandler(&s_stUart1Handle);
}


void MessageUartInit(void)
{
	/* for uart send and get a protocol message */
	IOFIFOInit(&s_stIOFIFOCtrl, s_stIOFIFOList, MAX_IO_FIFO_CNT, _IO_UART1);
	
	/* level one cache for protocol */
	LOCInit(&s_stLevelOneCache, s_c8LevelOneCache, LEVEL_ONE_CACHE_CNT * 2);
	
	/* for protocol analyse */
	CycleMsgInit(&s_stCycleBuf, s_c8CycleBuf, CYCLE_BUF_LENGTH);
	
	UARTInit();
}

StIOFIFO *MessageUartFlush(bool boSendALL)
{
	/* for data IN */ 
	do
	{
		void *pData = NULL;
		uint32_t u32Length = 0;
		pData = LOCCheckDataCanRead(&s_stLevelOneCache, &u32Length);
		
		if (pData == NULL)
		{
			/* no new data */
			break;
		}
		
		while(1)
		{
			void *pMsg = NULL;
			uint32_t u32GetMsgLength = 0;
			int32_t s32Protocol = 0;
			StIOFIFOList *pFIFO = NULL;
			pMsg = CycleGetOneMsg(&s_stCycleBuf, pData, u32Length, &u32GetMsgLength, 
					&s32Protocol, NULL);
			if (pMsg == NULL)
			{
				break;
			}
			/* I get some message */
			pFIFO = GetAUnusedFIFO(&s_stIOFIFOCtrl);
			if (pFIFO == NULL)
			{
				/* no buffer for this message */
				free(pMsg);
				break;
			}
			pFIFO->pData = pMsg;
			pFIFO->s32Length = u32GetMsgLength;
			pFIFO->boNeedFree = true;
			pFIFO->u8ProtocolType = s32Protocol;
			InsertIntoTheRWFIFO(&s_stIOFIFOCtrl, pFIFO, true);

			u32Length = 0;
		}
	} while (0);
	
	
	/* check message for send */
	do
	{
		static bool boHasSendAMessage = false;
		static StIOFIFOList stLastFIFO;
		StIOFIFOList *pFIFO = NULL;
		if (boHasSendAMessage)
		{
			if (HAL_UART_GetState(&s_stUart1Handle) != HAL_UART_STATE_READY)
			{
				if (boSendALL)
				{
					continue;	/* wait to finish to send this message */
				}
				else
				{
					break;
				}
			}	
			
			if (stLastFIFO.boNeedFree)
			{
				free(stLastFIFO.pData);
			}
			boHasSendAMessage = false;
		}
		pFIFO = GetAListFromRWFIFO(&s_stIOFIFOCtrl, false);
		if (pFIFO != NULL)
		{
			if ((pFIFO->s32Length <= 0) || (pFIFO->pData == NULL))
			{
				if ((pFIFO->boNeedFree) && (pFIFO->pData != NULL))
				{
					free(pFIFO->pData);
				}
				if (boSendALL)
				{
					continue;	/* wait to finish to send this message */
				}
				else
				{
					break;
				}				
			}
			  /*## Send the Buffer ###########################################*/
			if (HAL_UART_Transmit_DMA(&s_stUart1Handle, (uint8_t *)pFIFO->pData, pFIFO->s32Length) != HAL_OK)
			{
				/* Transfer error in transmission process */
				memset(&stLastFIFO, 0, sizeof(StIOFIFOList));
				free(pFIFO->pData);

			}
			else
			{
				boHasSendAMessage = true;
				
				stLastFIFO = *pFIFO;
			}
			
			ReleaseAUsedFIFO(&s_stIOFIFOCtrl, pFIFO);
			if (boSendALL)
			{
				continue;	/* send this message */
			}
		}
		break;
	} while (1);
	
	/* get message for read */
	do
	{
		StIOFIFOList *pFIFO = NULL;
		pFIFO = GetAListFromRWFIFO(&s_stIOFIFOCtrl, true);
		if (pFIFO != NULL)
		{
			return (StIOFIFO *)pFIFO;
		}

	} while (0);
	
	return NULL;
}


void MessageUartRelease(StIOFIFO *pFIFO)
{
	if (pFIFO != NULL)
	{
		if (pFIFO->boNeedFree)
		{
			free(pFIFO->pData);
		}
		ReleaseAUsedFIFO(&s_stIOFIFOCtrl, (StIOFIFOList *)pFIFO);
	}
}

int32_t MessageUartWrite(void *pData, bool boNeedFree, uint16_t u16ID, uint32_t u32Length)
{
	StIOFIFOList *pFIFO = NULL;
	if (pData == NULL)
	{
		return -1;
	}
	pFIFO = GetAUnusedFIFO(&s_stIOFIFOCtrl);
	if (pFIFO == NULL)
	{
		/* no buffer for this message */
		return -1;
	}
	pFIFO->pData = pData;
	pFIFO->s32Length = u32Length;
	pFIFO->boNeedFree = boNeedFree;
	InsertIntoTheRWFIFO(&s_stIOFIFOCtrl, pFIFO, false);
	
	return 0;
}

void MessageUartReleaseNoReleaseData(StIOFIFO *pFIFO)
{
	if (pFIFO != NULL)
	{
		ReleaseAUsedFIFO(&s_stIOFIFOCtrl, (StIOFIFOList *)pFIFO);
	}
}
int32_t GetMessageUartBufLength(void)
{
	return CYCLE_BUF_LENGTH;
}



const StIOTCB c_stUartIOTCB = 
{
	MessageUartInit,
	MessageUARTInitMsp,
	MessageUartFlush,
	MessageUartRelease,
	MessageUartReleaseNoReleaseData,
	GetMessageUartBufLength,
	MessageUartWrite,
};

