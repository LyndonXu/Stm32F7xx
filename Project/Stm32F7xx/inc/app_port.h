/******************(C) copyright 天津市XXXXX有限公司 *************************
* All Rights Reserved
* 文件名：app_port.h
* 摘要: 重要的驱动
* 版本：0.0.1
* 作者：许龙杰
* 日期：2013年05月08日
*******************************************************************************/
#ifndef _APP_PORT_H_
#define _APP_PORT_H_
#include "cpu.h"

#define  USE_CRITICAL()		CPU_SR_ALLOC()
#define  ENTER_CRITICAL()	CPU_INT_DIS()
#define  EXIT_CRITICAL()	CPU_INT_EN()


#endif
