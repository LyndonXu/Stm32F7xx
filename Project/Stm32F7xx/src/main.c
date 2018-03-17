/**
  ******************************************************************************
  * @file    GPIO/GPIO_IOToggle/Src/main.c
  * @author  MCD Application Team
  * @brief   This example describes how to configure and use GPIOs through
  *          the STM32F7xx HAL API.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sdram.h"
#include "main.h"
#include "os.h"

#include "message.h"
#include "protocol.h"
#include "common.h"

#include "bsp.h"
#include "os_app_hooks.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

/* Private functions ---------------------------------------------------------*/
int32_t CopyToUart1Message(void *pData, uint32_t u32Length);

#define  APP_CFG_TASK_START_PRIO                2u
#define  APP_CFG_TASK_OBJ_PRIO                  3u
#define  APP_CFG_TASK_EQ_PRIO                   4u


#define  APP_CFG_TASK_START_STK_SIZE            256u
#define  APP_CFG_TASK_EQ_STK_SIZE               512u
#define  APP_CFG_TASK_OBJ_STK_SIZE              256u


static  OS_TCB       AppTaskStartTCB;
static  CPU_STK      AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE];

static  void  AppTaskStart (void *p_arg)
{
    OS_ERR      err;
    CPU_INT32U  r0;
    CPU_INT32U  r1;
    CPU_INT32U  r2;
    CPU_INT32U  r3;
    CPU_INT32U  r4;
    CPU_INT32U  r5;
    CPU_INT32U  r6;
    CPU_INT32U  r7;
    CPU_INT32U  r8;
    CPU_INT32U  r9;
    CPU_INT32U  r10;
    CPU_INT32U  r11;
    CPU_INT32U  r12;


   (void)p_arg;

    r0  =  0u;                                                  /* Initialize local variables.                          */
    r1  =  1u;
    r2  =  2u;
    r3  =  3u;
    r4  =  4u;
    r5  =  5u;
    r6  =  6u;
    r7  =  7u;
    r8  =  8u;
    r9  =  9u;
    r10 = 10u;
    r11 = 11u;
    r12 = 12u;

    BSP_Init();                                                 /* Initialize BSP functions                             */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif


    BSP_LED_Off(0u);

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        BSP_LED_Toggle(0u);
        OSTimeDlyHMSM(0u, 0u, 0u, 100u,
                      OS_OPT_TIME_HMSM_STRICT,
                      &err);

        if ((r0  !=  0u) ||                                     /* Check task context.                                  */
            (r1  !=  1u) ||
            (r2  !=  2u) ||
            (r3  !=  3u) ||
            (r4  !=  4u) ||
            (r5  !=  5u) ||
            (r6  !=  6u) ||
            (r7  !=  7u) ||
            (r8  !=  8u) ||
            (r9  !=  9u) ||
            (r10 != 10u) ||
            (r11 != 11u) ||
            (r12 != 12u)) 
		{
			r12 = r12;
        }
    }
}

void OSTest()
{
	OS_ERR err = OS_ERR_NONE;
	/* Enable the CPU Cache */
	CPU_CACHE_Enable();

	/* STM32F7xx HAL library initialization:
	     - Configure the Flash prefetch
	     - Systick timer is configured by default as source of time base, but user
	       can eventually implement his proper time base source (a general purpose
	       timer for example or other time source), keeping in mind that Time base
	       duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	       handled in milliseconds basis.
	     - Set NVIC Group Priority to 4
	     - Low Level Initialization
	   */
	HAL_Init();

	/* Configure the system clock to 216 MHz */
	SystemClock_Config();


	CPU_IntDis();
	
    OSInit(&err);                                               /* Init uC/OS-III.                                      */
    App_OS_SetAllHooks();

    OSTaskCreate(&AppTaskStartTCB,                              /* Create the start task                                */
                  "App Task Start",
                  AppTaskStart,
                  0u,
                  APP_CFG_TASK_START_PRIO,
                 &AppTaskStartStk[0u],
                  AppTaskStartStk[APP_CFG_TASK_START_STK_SIZE / 10u],
                  APP_CFG_TASK_START_STK_SIZE,
                  0u,
                  0u,
                  0u,
                 (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 &err);

    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }	
}
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
	uint32_t u32TimeToggle = 0;
	uint32_t u32TimeToggle1S = 0;
	
	OSTest();
	
	/* This sample code shows how to use GPIO HAL API to toggle GPIOB-GPIO_PIN_0 IO
	  in an infinite loop. It is possible to connect a LED between GPIOB-GPIO_PIN_0
	  output and ground via a 330ohm resistor to see this external LED blink.
	  Otherwise an oscilloscope can be used to see the output GPIO signal */

	/* Enable the CPU Cache */
	CPU_CACHE_Enable();

	/* STM32F7xx HAL library initialization:
	     - Configure the Flash prefetch
	     - Systick timer is configured by default as source of time base, but user
	       can eventually implement his proper time base source (a general purpose
	       timer for example or other time source), keeping in mind that Time base
	       duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
	       handled in milliseconds basis.
	     - Set NVIC Group Priority to 4
	     - Low Level Initialization
	   */
	HAL_Init();

	/* Configure the system clock to 216 MHz */
	SystemClock_Config();
	
	
	SDRAM_Init();


	{
		GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
		
		/* -1- Enable GPIO Clock (to be able to program the configuration registers) */
		__HAL_RCC_GPIOB_CLK_ENABLE();
		
		/* -2- Configure IO in output push-pull mode to drive external LEDs */
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

		GPIO_InitStruct.Pin = GPIO_PIN_0;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);


	}
	
	{
		int32_t i;
		int32_t *pTmp = (int32_t *)(SDRAM_ADDR);
		for (i = 0; i < 1024; i++)
		{
			pTmp[i] = i;
		}
		for (i = 0; i < 1024; i++)
		{
			if (pTmp[i] != i)
			{
				while(1);
			}
			
		}
		
	}
	
	{
		int32_t LTDC_Init(void);
		LTDC_Init();
	}
	
	{
		c_stUartIOTCB.pFunMsgInit();
	}
	
	//u32TimeToggle1S = u32TimeToggle = HAL_GetTick();
	/* -3- Toggle IO in an infinite loop */
	while(1)
	{
		#if 1
		{
			void *pMsgIn = MessageUartFlush(false);
	
			if (pMsgIn != NULL)
			{
				if (BaseCmdProcess(pMsgIn, &c_stUartIOTCB) != 0)			
				{

				}
			}
			MessageUartRelease(pMsgIn);	

		}
		if (SysTimeDiff(u32TimeToggle1S, HAL_GetTick()) > 1000)
		{
			char c8Str[32];
			static int32_t tmp = 0;
			u32TimeToggle1S = HAL_GetTick();
			sprintf(c8Str, "test count: %d\n", tmp);
			
			if (CopyToUart1Message(c8Str, strlen(c8Str)) == 0)
			{
				tmp++;
			}
		}
		#endif
		if (SysTimeDiff(u32TimeToggle, HAL_GetTick()) > 100)
		{
			u32TimeToggle = HAL_GetTick();
			HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
			{
				static int32_t sx = 0;
				static int32_t sy = 0;
				static int32_t ex = 50;
				static int32_t ey = 50;
				
				static bool boXACC = true;
				static bool boYACC = true;
				
				void LTDC_Fill(uint16_t sx, uint16_t sy, 
					uint16_t ex, uint16_t ey, uint32_t u32Color);
				
				LTDC_Fill(sx, sy, ex, ey, 0xFFFFFFFF);
				if (boXACC)
				{
					if (ex < 480 - 1)
					{
						sx++;
						ex++;
					}
					else
					{
						boXACC = false;
					}
				}
				else
				{
					if (sx != 0)
					{
						sx--;
						ex--;					
					}
					else
					{
						boXACC = true;
					}
				}
				
				if (boYACC)
				{
					if (ey < 272 - 1)
					{
						sy++;
						ey++;
					}
					else
					{
						boYACC = false;
					}
				}
				else
				{
					if (sy != 0)
					{
						sy--;
						ey--;					
					}
					else
					{
						boYACC = true;
					}
				}

				LTDC_Fill(sx, sy, ex, ey, 0xFFFF0000);
			}
		}
	}
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            PLL_R                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;
	RCC_OscInitStruct.PLL.PLLR = 7;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		while(1) {};
	}

	/* Activate the OverDrive to reach the 216 Mhz Frequency */
	if(HAL_PWREx_EnableOverDrive() != HAL_OK)
	{
		while(1) {};
	}


	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
	   clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		while(1) {};
	}
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
	
	SCB->CACR |= SCB_CACR_FORCEWT_Msk;
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
	   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while(1)
	{
	}
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
