/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：protocol.h
* 摘要: 协议自动检测程序
* 版本：0.0.1
* 作者：许龙杰
* 日期：2013年01月25日
*******************************************************************************/
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>

#include "app_port.h"
#include "IOCtrl.h"


#define ALIGN(org, align)					(((org + (align - 1)) / align) * align)

#define PROTOCOL_MAX_LENGTH					64
#if PROTOCOL_MAX_LENGTH < PROTOCOL_YNA_ENCODE_LENGTH
#undefine PROTOCOL_MAX_LENGTH
#define PROTOCOL_MAX_LENGTH					ALIGN(PROTOCOL_YNA_ENCODE_LENGTH, 4)
#endif

#if	PROTOCOL_MAX_LENGTH < PROTOCOL_NJ_LENGTH
#undefine PROTOCOL_MAX_LENGTH
#define PROTOCOL_MAX_LENGTH					ALIGN(PROTOCOL_NJ_LENGTH, 4)
#endif

typedef enum _tagEmMainState
{
	_Main_State_Sync,
	_Main_State_Normal,
	
}EmMainState;


#define _YNA_CAM_RIGHT		0x01
#define _YNA_CAM_LEFT		0x02
#define _YNA_CAM_UP			0x04
#define _YNA_CAM_DOWN		0x08
#define _YNA_CAM_TELE		0x10
#define _YNA_CAM_WIDE		0x20


enum
{
	_IO_UART1,
	_IO_UART2,
	_IO_USB,
	
	_IO_Reserved,
};

enum
{
	_YNA_Sync,
	_YNA_Addr,
	_YNA_Mix,
	_YNA_Cmd,
	_YNA_Data1,
	_YNA_Data2,
	_YNA_Data3,
	_YNA_CheckSum,
};

enum
{
	_PELCOD_Sync,
	_PELCOD_Addr,
	_PELCOD_Cmd1,
	_PELCOD_Cmd2,
	_PELCOD_Data1,
	_PELCOD_Data2,
	_PELCOD_CheckSum,
};

enum
{
	_BJZS_Special,
	_BJZS_Key,
	_BJZS_Extern,
};


#define PELCOD_ZOOM_WIDE	0x40
#define PELCOD_ZOOM_TELE	0x20
#define PELCOD_DOWN			0x10
#define PELCOD_UP			0x08
#define PELCOD_LEFT			0x04
#define PELCOD_RIGHT		0x02



#define PROTOCOL_YNA_ENCODE_LENGTH			10	
#define PROTOCOL_YNA_DECODE_LENGTH			8	
#define PROTOCOL_NJ_LENGTH					8	/* 协议长度 */
#define PROTOCOL_PELCO_D_LENGTH				7	
#define PROTOCOL_PELCO_P_LENGTH				8	
#define PROTOCOL_VISCA_MIN_LENGTH			3	
#define PROTOCOL_VISCA_MAX_LENGTH			16	
#define PROTOCOL_RQ_LENGTH					7	
#define PROTOCOL_SB_LENGTH					11	
#define PROTOCOL_BJZS_LENGTH				3	


typedef enum _tagEmProtocol
{
	_Protocol_ZS,
	_Protocol_YNA,
	_Protocol_PELCO_D,
	_Protocol_PecloD = _Protocol_PELCO_D,
	_Protocol_PELCO_P,
	_Protocol_VISCA,
	_Protocol_Visca = _Protocol_VISCA,
	_Protocol_RQ,
	_Protocol_SB,
	_Protocol_SB_HID,
	_Protocol_BJZS,
	_Protocol_BJZS_HID,
	
	_Protocol_Reserved,	
}EmProtocol;

extern EmProtocol g_emProtocol;

#define PRINT_MFC(x, ...)

typedef struct _tagStCycleBuf
{
	char *pBuf;
	uint32_t u32TotalLength;
	uint32_t u32Write;
	uint32_t u32Read;
	uint32_t u32Using;
	uint32_t u32Flag;
}StCycleBuf;


int32_t CycleMsgInit(StCycleBuf *pCycleBuf, void *pBuf, uint32_t u32Length);
void *CycleGetOneMsg(StCycleBuf *pCycleBuf, const char *pData, 
	uint32_t u32DataLength, uint32_t *pLength, int32_t *pProtocolType, int32_t *pErr);
void *YNAMakeAnArrayVarialbleCmd(uint16_t u16Cmd, void *pData,
	uint32_t u32Count, uint32_t u32Length, uint32_t *pCmdLength);
void *YNAMakeASimpleVarialbleCmd(uint16_t u16Cmd, void *pData, 
	uint32_t u32DataLength, uint32_t *pCmdLength);



void ChangeEncodeState(void);
void YNADecode(uint8_t *pBuf);
void YNAEncodeAndGetCheckSum(uint8_t *pBuf);
void YNAGetCheckSum(uint8_t *pBuf);

int32_t BaseCmdProcess(StIOFIFO *pFIFO, const StIOTCB *pIOTCB);

bool KeyProcess(StIOFIFO *pFIFO);
bool PCEchoProcess(StIOFIFO *pFIFO);

void TurnOnSwitchLed(void);
bool ProtocolSelect(StIOFIFO *pFIFO);

void CopyToUartMessage(void *pData, uint32_t u32Length);


#endif


