#include <stdint.h>
#include "stm32f7xx_hal.h"
#include "message.h"
/**
  * @brief UART MSP Initialization
  *        This function configures the hardware resources used in this example:
  *           - Peripheral's clock enable
  *           - Peripheral's GPIO Configuration
  *           - DMA configuration for transmission request by peripheral
  *           - NVIC configuration for DMA interrupt request enable
  * @param huart: UART handle pointer
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	if(huart == NULL)
	{
		return;
	}
	if(huart->Instance == USART1)
	{
		c_stUartIOTCB.pFunMsgInitMsp();
	}
}



