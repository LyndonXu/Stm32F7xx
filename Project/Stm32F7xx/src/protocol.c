/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：protocol.c
* 摘要: 协议控制程序
* 版本：0.0.1
* 作者：许龙杰
* 日期：2013年01月25日
*******************************************************************************/
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f7xx_hal.h"
	
#include "protocol.h"

#include "message.h"

#include "common.h"


#define APP_VERSOIN	"STM32F7XX_20180314"


int32_t CycleMsgInit(StCycleBuf *pCycleBuf, void *pBuf, uint32_t u32Length)
{
	if ((pCycleBuf == NULL) || (pBuf == NULL) || (u32Length == 0))
	{
		return -1;
	}
	memset(pCycleBuf, 0, sizeof(StCycleBuf));
	pCycleBuf->pBuf = pBuf;
	pCycleBuf->u32TotalLength = u32Length;
	
	return 0;
}

void *CycleGetOneMsg(StCycleBuf *pCycleBuf, const char *pData, 
	uint32_t u32DataLength, uint32_t *pLength, int32_t *pProtocolType, int32_t *pErr)
{
	char *pBuf = NULL;
	int32_t s32Err = 0;
	if ((pCycleBuf == NULL) || (pLength == NULL))
	{
		s32Err = -1;
		goto end;
	}
	if (((pCycleBuf->u32TotalLength - pCycleBuf->u32Using) < u32DataLength)
		/*|| (u32DataLength == 0)*/)
	{
		PRINT_MFC("data too long\n");
		s32Err = -1;
	}
	if (u32DataLength != 0)
	{
		if (pData == NULL)
		{
			s32Err = -1;
			goto end;
		}
		else	/* copy data */
		{
			uint32_t u32Tmp = pCycleBuf->u32Write + u32DataLength;
			if (u32Tmp > pCycleBuf->u32TotalLength)
			{
				uint32_t u32CopyLength = pCycleBuf->u32TotalLength - pCycleBuf->u32Write;
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32CopyLength);
				memcpy(pCycleBuf->pBuf, pData + u32CopyLength, u32DataLength - u32CopyLength);
				pCycleBuf->u32Write = u32DataLength - u32CopyLength;
			}
			else
			{
				memcpy(pCycleBuf->pBuf + pCycleBuf->u32Write, pData, u32DataLength);
				pCycleBuf->u32Write += u32DataLength;
			}
			pCycleBuf->u32Using += u32DataLength;

		}
	}

	do
	{
		uint32_t i;
		bool boIsBreak = false;

		for (i = 0; i < pCycleBuf->u32Using; i++)
		{
			uint32_t u32ReadIndex = i + pCycleBuf->u32Read;
			char c8FirstByte;
			u32ReadIndex %= pCycleBuf->u32TotalLength;
			c8FirstByte = pCycleBuf->pBuf[u32ReadIndex];
			if (c8FirstByte == ((char)0xAA))
			{
				#define YNA_NORMAL_CMD		0
				#define YNA_VARIABLE_CMD	1 /*big than PROTOCOL_YNA_DECODE_LENGTH */
				uint32_t u32MSB = 0;
				uint32_t u32LSB = 0;
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				
				/* check whether it's a variable length command */
				if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH - 1)
				{
					if (pCycleBuf->u32Flag != YNA_NORMAL_CMD)
					{
						u32MSB = ((pCycleBuf->u32Flag >> 8) & 0xFF);
						u32LSB = ((pCycleBuf->u32Flag >> 0) & 0xFF);
					}
					else
					{
						uint32_t u32Start = i + pCycleBuf->u32Read;
						char *pTmp = pCycleBuf->pBuf;
						if ((pTmp[(u32Start + _YNA_Mix) % pCycleBuf->u32TotalLength] == 0x04)
							&& (pTmp[(u32Start + _YNA_Cmd) % pCycleBuf->u32TotalLength] == 0x00))
						{
							u32MSB = pTmp[(u32Start + _YNA_Data2) % pCycleBuf->u32TotalLength];
							u32LSB = pTmp[(u32Start + _YNA_Data3) % pCycleBuf->u32TotalLength];
							if (s32RemainLength >= PROTOCOL_YNA_DECODE_LENGTH)
							{
								uint32_t u32Start = i + pCycleBuf->u32Read;
								uint32_t u32End = PROTOCOL_YNA_DECODE_LENGTH - 1 + i + pCycleBuf->u32Read;
								char c8CheckSum = 0;
								uint32_t j;
								char c8Tmp;
								for (j = u32Start; j < u32End; j++)
								{
									c8CheckSum ^= pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
								}
								c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
								if (c8CheckSum != c8Tmp) /* wrong message */
								{
									PRINT_MFC("get a wrong command: %d\n", u32MSB);
									pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
									pCycleBuf->u32Read += (i + 1);
									pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
									break;
								}
								
								u32MSB &= 0xFF;
								u32LSB &= 0xFF;
								
								pCycleBuf->u32Flag = ((u32MSB << 8) + u32LSB);
							}
						}
					}
				}
				u32MSB &= 0xFF;
				u32LSB &= 0xFF;
				u32MSB = (u32MSB << 8) + u32LSB;
				u32MSB += PROTOCOL_YNA_DECODE_LENGTH;
				PRINT_MFC("the data length is: %d\n", u32MSB);
				if (u32MSB > (pCycleBuf->u32TotalLength / 2))	/* maybe the message is wrong */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
					pCycleBuf->u32Read += (i + 1);
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					pCycleBuf->u32Flag = 0;
				}
				else if (((int32_t)(u32MSB)) <= s32RemainLength) /* good, I may got a message */
				{
					if (u32MSB == PROTOCOL_YNA_DECODE_LENGTH)
					{
						char c8CheckSum = 0, *pBufTmp, c8Tmp;
						uint32_t j, u32Start, u32End;
						uint32_t u32CmdLength = u32MSB;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						pBufTmp = pBuf;
						u32Start = i + pCycleBuf->u32Read;

						u32End = u32MSB - 1 + i + pCycleBuf->u32Read;
						PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
						for (j = u32Start; j < u32End; j++)
						{
							c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							c8CheckSum ^= c8Tmp;
							*pBufTmp++ = c8Tmp;
						}
						c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
						if (c8CheckSum == c8Tmp) /* good message */
						{
							boIsBreak = true;
							*pBufTmp = c8Tmp;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							pCycleBuf->u32Flag = 0;
						}
					}
					else /* variable length */
					{
						uint32_t u32Start, u32End;
						uint32_t u32CmdLength = u32MSB;
						uint16_t u16CRCModBus;
						uint16_t u16CRCBuf;
						pBuf = (char *)malloc(u32CmdLength);
						if (pBuf == NULL)
						{
							s32Err = -1; /* big problem */
							goto end;
						}
						u32Start = (i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						u32End = (u32MSB + i + pCycleBuf->u32Read) % pCycleBuf->u32TotalLength;
						PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
						if (u32End > u32Start)
						{
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32MSB);
						}
						else
						{
							uint32_t u32FirstCopy = pCycleBuf->u32TotalLength - u32Start;
							memcpy(pBuf, pCycleBuf->pBuf + u32Start, u32FirstCopy);
							memcpy(pBuf + u32FirstCopy, pCycleBuf->pBuf, u32MSB - u32FirstCopy);
						}

						pCycleBuf->u32Flag = YNA_NORMAL_CMD;
						
						/* we need not check the head's check sum,
						 * just check the CRC16-MODBUS
						 */
						u16CRCModBus = CRC16((const uint8_t *)pBuf + PROTOCOL_YNA_DECODE_LENGTH, 
							u32MSB - PROTOCOL_YNA_DECODE_LENGTH - 2);
						u16CRCBuf = 0;

						LittleAndBigEndianTransfer((char *)(&u16CRCBuf), pBuf + u32MSB - 2, 2);
						if (u16CRCBuf == u16CRCModBus) /* good message */
						{
							boIsBreak = true;

							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_YNA;
							}
						}
						else
						{
							free(pBuf);
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						}
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			else if(c8FirstByte == ((char)0xFA))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_RQ_LENGTH)
				{
					volatile char c8CheckSum = 0, *pBufTmp, c8Tmp;
					volatile uint32_t j, u32Start, u32End;
					volatile uint32_t u32CmdLength = PROTOCOL_RQ_LENGTH;
					pBuf = (char *)malloc(u32CmdLength);
					if (pBuf == NULL)
					{
						s32Err = -1; /* big problem */
						goto end;
					}
					pBufTmp = pBuf;
					u32Start = i + pCycleBuf->u32Read;

					u32End = PROTOCOL_RQ_LENGTH - 1 + i + pCycleBuf->u32Read;
					PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						*pBufTmp++ = c8Tmp;
					}
					
					for (j = 2; j < PROTOCOL_RQ_LENGTH - 1; j++)
					{
						c8CheckSum += pBuf[j];
					}
					
					
					c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
					if (c8CheckSum == c8Tmp) /* good message */
					{
						boIsBreak = true;
						*pBufTmp = c8Tmp;

						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
						pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						PRINT_MFC("get a command: %d\n", u32MSB);
						PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
						*pLength = u32CmdLength;
						if (pProtocolType != NULL)
						{
							*pProtocolType = _Protocol_RQ;
						}
					}
					else
					{
						free(pBuf);
						pBuf = NULL;
						PRINT_MFC("get a wrong command: %d\n", u32MSB);
						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
						pCycleBuf->u32Read += (i + 1);
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			
			else if((c8FirstByte & 0xF0) == ((char)0x80))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_VISCA_MIN_LENGTH)
				{
					uint32_t j;
					uint32_t u32Start = i + pCycleBuf->u32Read;
					uint32_t u32End = pCycleBuf->u32Using + pCycleBuf->u32Read;
					char c8Tmp = 0;
					for (j = u32Start + PROTOCOL_VISCA_MIN_LENGTH - 1; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						if (c8Tmp == (char)0xFF)
						{
							u32End = j;
							break;
						}
					}
					if (c8Tmp == (char)0xFF)
					{
						/* wrong message */
						if ((u32End - u32Start + 1) > PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;							
						}
						else
						{
							char *pBufTmp;
							uint32_t u32CmdLength = u32End - u32Start + 1;
							pBuf = (char *)malloc(u32CmdLength);
							if (pBuf == NULL)
							{
								s32Err = -1; /* big problem */
								goto end;
							}	
							pBufTmp = pBuf;
							boIsBreak = true;
							
							PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
							for (j = u32Start; j <= u32End; j++)
							{
								*pBufTmp++ = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
							}
							
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
							pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							PRINT_MFC("get a command: %d\n", u32MSB);
							PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
							*pLength = u32CmdLength;
							if (pProtocolType != NULL)
							{
								*pProtocolType = _Protocol_VISCA;
							}

						}
					}
					else
					{
						/* wrong message */
						if ((u32End - u32Start) >= PROTOCOL_VISCA_MAX_LENGTH)
						{
							pBuf = NULL;
							PRINT_MFC("get a wrong command: %d\n", u32MSB);
							pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
							pCycleBuf->u32Read += (i + 1);
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;							
						}
						else
						{
							pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
							pCycleBuf->u32Read += i;
							pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
							boIsBreak = true;						
						}
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
			else if(c8FirstByte == ((char)0xA5))
			{
				int32_t s32RemainLength = pCycleBuf->u32Using - i;
				if (s32RemainLength >= PROTOCOL_SB_LENGTH)
				{
					volatile char c8CheckSum = 0, *pBufTmp, c8Tmp;
					volatile uint32_t j, u32Start, u32End;
					volatile uint32_t u32CmdLength = PROTOCOL_SB_LENGTH;
					pBuf = (char *)malloc(u32CmdLength);
					if (pBuf == NULL)
					{
						s32Err = -1; /* big problem */
						goto end;
					}
					pBufTmp = pBuf;
					u32Start = i + pCycleBuf->u32Read;

					u32End = PROTOCOL_SB_LENGTH - 1 + i + pCycleBuf->u32Read;
					PRINT_MFC("start: %d, end: %d\n", u32Start, u32End);
					for (j = u32Start; j < u32End; j++)
					{
						c8Tmp = pCycleBuf->pBuf[j % pCycleBuf->u32TotalLength];
						*pBufTmp++ = c8Tmp;
					}
					
					for (j = 1; j < PROTOCOL_SB_LENGTH - 1; j++)
					{
						c8CheckSum += pBuf[j];
					}
					
					
					c8Tmp = pCycleBuf->pBuf[u32End % pCycleBuf->u32TotalLength];
					if (c8CheckSum == c8Tmp) /* good message */
					{
						boIsBreak = true;
						*pBufTmp = c8Tmp;

						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + u32CmdLength));
						pCycleBuf->u32Read = i + pCycleBuf->u32Read + u32CmdLength;
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
						PRINT_MFC("get a command: %d\n", u32MSB);
						PRINT_MFC("u32Using: %d, u32Read: %d, u32Write: %d\n", pCycleBuf->u32Using, pCycleBuf->u32Read, pCycleBuf->u32Write);
						*pLength = u32CmdLength;
						if (pProtocolType != NULL)
						{
							*pProtocolType = _Protocol_SB;
						}
					}
					else
					{
						free(pBuf);
						pBuf = NULL;
						PRINT_MFC("get a wrong command: %d\n", u32MSB);
						pCycleBuf->u32Using = (pCycleBuf->u32Using - (i + 1));
						pCycleBuf->u32Read += (i + 1);
						pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					}
				}
				else	/* message not enough long */
				{
					pCycleBuf->u32Using = (pCycleBuf->u32Using - i);
					pCycleBuf->u32Read += i;
					pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
					boIsBreak = true;
				}
				break;
			}
		}
		if ((i == pCycleBuf->u32Using) && (!boIsBreak))
		{
			PRINT_MFC("cannot find AA, i = %d\n", pCycleBuf->u32Using);
			pCycleBuf->u32Using = 0;
			pCycleBuf->u32Read += i;
			pCycleBuf->u32Read %= pCycleBuf->u32TotalLength;
			pCycleBuf->u32Flag = 0;
		}

		if (boIsBreak)
		{
			break;
		}
	} while (((int32_t)pCycleBuf->u32Using) > 0);

	//if (pCycleBuf->u32Write + u32DataLength)

end:
	if (pErr != NULL)
	{
		*pErr = s32Err;
	}
	return pBuf;
}
void *YNAMakeAnArrayVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32Count, uint32_t u32Length, uint32_t *pCmdLength)
{
	uint32_t u32CmdLength;
	uint32_t u32DataLength;
	uint32_t u32Tmp;
	uint8_t *pCmd = NULL;
	uint8_t *pVarialbleCmd;
	if (pData == NULL)
	{
		return NULL;
	}
	
	u32DataLength = u32Count * u32Length;
	
	/*  */
	u32CmdLength = PROTOCOL_YNA_DECODE_LENGTH + 6 + u32DataLength + 2;
	pCmd = malloc(u32CmdLength);
	if (pCmd == NULL)
	{
		return NULL;
	}
	memset(pCmd, 0, u32CmdLength);
	pCmd[_YNA_Sync] = 0xAA;
	pCmd[_YNA_Mix] = 0x04;
	pCmd[_YNA_Cmd] = 0x00;
	
	/* total length */
	u32Tmp = u32CmdLength - PROTOCOL_YNA_DECODE_LENGTH;
	LittleAndBigEndianTransfer((char *)pCmd + _YNA_Data2, (const char *)(&u32Tmp), 2);
	
	YNAGetCheckSum(pCmd);
	
	pVarialbleCmd = pCmd + PROTOCOL_YNA_DECODE_LENGTH;
	
	/* command serial */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd, (const char *)(&u16Cmd), 2);

	/* command count */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 2, (const char *)(&u32Count), 2);

	/* Varialble data length */
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 4, (const char *)(&u32Length), 2);
	
	/* copy the data */
	memcpy(pVarialbleCmd + 6, pData, u32DataLength);

	/* get the CRC16 of the variable command */
	u32Tmp = CRC16(pVarialbleCmd, 6 + u32DataLength);
	
	LittleAndBigEndianTransfer((char *)pVarialbleCmd + 6 + u32DataLength, 
		(const char *)(&u32Tmp), 2);

	if (pCmdLength != NULL)
	{
		*pCmdLength = u32CmdLength;
	}
	
	return pCmd;
}

void *YNAMakeASimpleVarialbleCmd(uint16_t u16Cmd, void *pData, 
	uint32_t u32DataLength, uint32_t *pCmdLength)
{
	return YNAMakeAnArrayVarialbleCmd(u16Cmd, pData, 1, u32DataLength, pCmdLength);
}

int32_t CopyToUart1Message(void *pData, uint8_t u32Length)
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
				return -1;
			}
			return 0;
		}
		return -1;
	}
	return -1;
}

int32_t BaseCmdProcess(StIOFIFO *pFIFO, const StIOTCB *pIOTCB)
{
	uint8_t *pMsg;
	bool boGetVaildBaseCmd = true;
	if (pFIFO == NULL)
	{
		return -1;
	}
	pMsg = (uint8_t *)pFIFO->pData;
	
	if (pMsg[_YNA_Sync] != 0xAA)
	{
		return -1;
	}
	
	if (pMsg[_YNA_Mix] == 0x0C)
	{
		if (pMsg[_YNA_Cmd] == 0x80)
		{
			uint8_t u8EchoBase[PROTOCOL_YNA_DECODE_LENGTH] = {0};
			uint8_t *pEcho = NULL;
			uint32_t u32EchoLength = 0;
			bool boHasEcho = true;
			bool boNeedCopy = true;
			bool boNeedReset = false;
			u8EchoBase[_YNA_Sync] = 0xAA;
			u8EchoBase[_YNA_Mix] = 0x0C;
			u8EchoBase[_YNA_Cmd] = 0x80;
			u8EchoBase[_YNA_Data1] = 0x01;
			switch(pMsg[_YNA_Data3])
			{
				case 0x01:	/* just echo the same command */
				{
					//SetOptionByte(OPTION_UPGRADE_DATA);
					//boNeedReset = true;
				}
				case 0x02:	/* just echo the same command */
				{
					u8EchoBase[_YNA_Data3] = pMsg[_YNA_Data3];
					pEcho = (uint8_t *)malloc(PROTOCOL_YNA_DECODE_LENGTH);
					if (pEcho == NULL)
					{
						boHasEcho = false;
						break;
					}
					u32EchoLength = PROTOCOL_YNA_DECODE_LENGTH;						
					break;
				}
				case 0x03:	/* return the UUID */
				{
					/* <TODO:  > */
#define GET_UID_CNT(Byte)			(96 / (Byte * 8))
#define UID_BASE_ADDR				(0x1FFFF420)
					
#pragma anon_unions
typedef struct _tagStUID
{
	union
	{
		uint16_t u16UID[GET_UID_CNT(sizeof(uint16_t))];
		uint32_t u32UID[GET_UID_CNT(sizeof(uint32_t))];
		uint8_t u8UID[GET_UID_CNT(sizeof(uint8_t))];
	};
}StUID;
					StUID stUID;
					//GetUID(&stUID);
					memcpy(&stUID, (void *)UID_BASE_ADDR, sizeof(StUID));
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x8003, 
							&stUID, sizeof(StUID), &u32EchoLength);
					
					//boHasEcho = false;
					break;
				}
				case 0x05:	/* return the BufLength */
				{
					uint16_t u16BufLength = 0;
					if (pIOTCB != NULL
						 && pIOTCB->pFunGetMsgBufLength != 0)
					{
						u16BufLength = pIOTCB->pFunGetMsgBufLength();
					}
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x8005, 
							&u16BufLength, sizeof(uint16_t), &u32EchoLength);
					break;
				}
				case 0x09:	/* RESET the MCU */
				{
					NVIC_SystemReset();
					boHasEcho = false;
					break;
				}
				case 0x0B:	/* Get the application's CRC32 */
				{
					/* <TODO: > 
					uint32_t u32CRC32 = ~0;
					u32CRC32 = AppCRC32(~0);
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x800B, 
							&u32CRC32, sizeof(uint32_t), &u32EchoLength);*/
					boHasEcho = false;
					break;
				}
				case 0x0C:	/* get the version */
				{
					const char *pVersion = APP_VERSOIN;
					boNeedCopy = false;
					pEcho = YNAMakeASimpleVarialbleCmd(0x800C, 
							(void *)pVersion, strlen(pVersion) + 1, &u32EchoLength);
					break;
				}
				
				default:
					boHasEcho = false;
					boGetVaildBaseCmd = false;
					break;
			}
			if (boHasEcho && pEcho != NULL)
			{
				if (boNeedCopy)
				{
					YNAGetCheckSum(u8EchoBase);
					memcpy(pEcho, u8EchoBase, PROTOCOL_YNA_DECODE_LENGTH);
				}
				if (pIOTCB == NULL)
				{
					free(pEcho);
				}
				else if (pIOTCB->pFunMsgWrite == NULL)
				{
					free(pEcho);			
				}
				else if(pIOTCB->pFunMsgWrite(pEcho, true, _IO_Reserved, u32EchoLength) != 0)
				{
					free(pEcho);
				}
			}
			
			/* send all the command in the buffer */
			if (boNeedReset)
			{
				//MessageUartFlush(true); 
				NVIC_SystemReset();
			}
		}
	}
	else if (pMsg[_YNA_Mix] == 0x04)	/* variable command */
	{
		uint32_t u32TotalLength = 0;
		uint32_t u32ReadLength = 0;
		uint8_t *pVariableCmd;
		
		boGetVaildBaseCmd = false;
		
		/* get the total command length */
		LittleAndBigEndianTransfer((char *)(&u32TotalLength), (const char *)pMsg + _YNA_Data2, 2);
		pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH;
		u32TotalLength -= 2; /* CRC16 */
		while (u32ReadLength < u32TotalLength)
		{
			uint8_t *pEcho = NULL;
			uint32_t u32EchoLength = 0;
			bool boHasEcho = true;
			uint16_t u16Command = 0, u16Count = 0, u16Length = 0;
			LittleAndBigEndianTransfer((char *)(&u16Command),
				(char *)pVariableCmd, 2);
			LittleAndBigEndianTransfer((char *)(&u16Count),
				(char *)pVariableCmd + 2, 2);
			LittleAndBigEndianTransfer((char *)(&u16Length),
				(char *)pVariableCmd + 4, 2);

			switch (u16Command)
			{
				case 0x800A:
				{
					/* <TODO:> check the crc32 and UUID, and BTEA and check the number 
					int32_t s32Err;
					char *pData = (char *)pVariableCmd + 6;
					StBteaKey stLic;
					StUID stUID;
					uint32_t u32CRC32;
					
					GetUID(&stUID);
					u32CRC32 = AppCRC32(~0);
					GetLic(&stLic, &stUID, u32CRC32, true);
					
					if (memcmp(&stLic, pData, sizeof(StBteaKey)) == 0)
					{
						s32Err = 0;
						
						WriteLic(&stLic, true, 0);
					}
					else
					{
						s32Err = -1;
					}
					pEcho = YNAMakeASimpleVarialbleCmd(0x800A, &s32Err, 4, &u32EchoLength);
					*/
					boGetVaildBaseCmd = true;
					break;
				}
				default:
					break;
			}
			
			if (boHasEcho && pEcho != NULL)
			{
				if (pIOTCB == NULL)
				{
					free(pEcho);
				}
				else if (pIOTCB->pFunMsgWrite == NULL)
				{
					free(pEcho);
				}
				else if(pIOTCB->pFunMsgWrite(pEcho, true, _IO_Reserved, u32EchoLength) != 0)
				{
					free(pEcho);
				}
			}
			
			
			u32ReadLength += (6 + (uint32_t)u16Count * u16Length);
			pVariableCmd = pMsg + PROTOCOL_YNA_DECODE_LENGTH + u32ReadLength;
		}
	}
	else
	{
		boGetVaildBaseCmd = false;
	}

	return boGetVaildBaseCmd ? 0: -1;
}


void ChangeEncodeState(void)
{

}

void YNADecode(uint8_t *pBuf)
{

}
void YNAEncodeAndGetCheckSum(uint8_t *pBuf)
{
}


void YNAGetCheckSum(uint8_t *pBuf)
{
	int32_t i, s32End;
	uint8_t u8Sum = pBuf[0];

	s32End = PROTOCOL_YNA_DECODE_LENGTH - 1;
	for (i = 1; i < s32End; i++)
	{
		u8Sum ^= pBuf[i];
	}
	pBuf[i] = u8Sum;
}

void PelcoDGetCheckSum(uint8_t *pBuf)
{
	int32_t i;
	uint8_t u8Sum = 0;
	for (i = 1; i < 6; i++)
	{
		u8Sum += pBuf[i];
	}
	pBuf[i] = u8Sum;
}

void SBGetCheckSum(uint8_t *pBuf)
{
	int32_t i;
	uint8_t u8Sum = 0;
	for (i = 1; i < PROTOCOL_SB_LENGTH - 1; i++)
	{
		u8Sum += pBuf[i];
	}
	pBuf[i] = u8Sum;
}

