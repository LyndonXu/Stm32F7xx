/******************(C) copyright �����XXXXX���޹�˾ *************************
* All Rights Reserved
* �ļ�����app_port.h
* ժҪ: ��Ҫ������
* �汾��0.0.1
* ���ߣ�������
* ���ڣ�2013��05��08��
*******************************************************************************/
#ifndef _APP_PORT_H_
#define _APP_PORT_H_
#include "cpu.h"

#define  USE_CRITICAL()		CPU_SR_ALLOC()
#define  ENTER_CRITICAL()	CPU_INT_DIS()
#define  EXIT_CRITICAL()	CPU_INT_EN()


#endif
