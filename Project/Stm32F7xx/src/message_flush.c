/******************(C) copyright �����XXXXX���޹�˾ *************************
* All Rights Reserved
* �ļ�����messageflush.c
* ժҪ: UART, CAN Э������Լ�������
* �汾��0.0.1
* ���ߣ�������
* ���ڣ�2016��08��25��
*******************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f7xx_hal.h"

#include <os.h>

#include "common.h"
#include "protocol.h"
#include "message.h"

//#include "message_flush.h"

static OS_SEM s_stMessageSem;
static OS_SEM *s_pMessageSem = &s_stMessageSem;


void TriggerMessageFlush(void)
{
	if (s_pMessageSem != NULL)
	{
		OS_ERR emErr = OS_ERR_NONE;
		OSSemPost(s_pMessageSem, OS_OPT_POST_1, &emErr);
	}
}


void IRQTriggerMessageFlush(void)
{
	OSIntEnter();

	TriggerMessageFlush();
	
	OSIntExit();
}


void CopyToUartMessge(void *pData, uint32_t u32Length)
{
	if ((pData != NULL) && (u32Length != 0))
	{
		void *pBuf = malloc(u32Length);
		if (pBuf != NULL)
		{
			memcpy(pBuf, pData, u32Length);
			if (MessageUartWrite(pBuf, true, _IO_Reserved, u32Length) != 0)
			{
				free (pBuf);
			}	
		}
	}
}



void TaskMessageFlush(void *pArg)
{
	OS_ERR emErr = OS_ERR_NONE;
	OSSemCreate(s_pMessageSem, 0, NULL, &emErr);
	while(1)
	{
		StIOFIFO *pMsgIn;
		/* get a sem or timeout */
		OSSemPend(s_pMessageSem, 20, OS_OPT_PEND_BLOCKING, NULL, &emErr);
		
		pMsgIn = MessageUartFlush(false);
		
		if (pMsgIn != NULL)
		{
			if (BaseCmdProcess(pMsgIn, &c_stUartIOTCB) == 0)
			{
			}
			MessageUartRelease(pMsgIn);		
		}
		
#if 0		
		do
		{
			static u8 u8Buf[] = {1, 2, 3, 4, 5, 6, 7, 8};
			static u32 u32TimeOrg = 0;
			if (SysTimeDiff(u32TimeOrg, OSTimeGet()) > 500)
			{
				u32TimeOrg = OSTimeGet();
				MessageCanWrite(u8Buf, false, _IO_CAN_AA00, 8);			
			}
			
		}while(0);
#endif
	}
}
