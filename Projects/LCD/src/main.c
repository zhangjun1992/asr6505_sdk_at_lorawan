/**
  ******************************************************************************
  * @file    USART/USART_Printf/main.c
  * @author  MCD Application Team
  * @version V1.5.2
  * @date    30-September-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
//#include "board.h"
//#include "gpio-board.h"
#include "stm8l15x_rtc.h"
#include "stm8l15x_tim1.h"
#include "stm8l15x_iwdg.h"
#include "stm8l15x_wwdg.h"
#include "stm8l15x_aes.h"
#include "stdio.h"
//#include "timer.h"
#include "stm8l1528_glass_lcd.h"
#include "stm8l15x_lcd.h"
#include "stm8l15x_tim4.h"
//#include "rtc-board.h"

/** @addtogroup STM8L15x_StdPeriph_Examples
  * @{
  */

/**
  * @addtogroup USART_Printf
  * @{
  */


/* Private functions ---------------------------------------------------------*/

//HW 

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

static void BoardUnusedIoInit( void );
static void SystemClockConfig( void );

#ifdef _RAISONANCE_
#define PUTCHAR_PROTOTYPE int putchar (char c)
#define GETCHAR_PROTOTYPE int getchar (void)
#elif defined (_COSMIC_)
#define PUTCHAR_PROTOTYPE char putchar (char c)
#define GETCHAR_PROTOTYPE char getchar (void)
#else /* _IAR_ */
#define PUTCHAR_PROTOTYPE int putchar (int c)
#define GETCHAR_PROTOTYPE int getchar (void)
#endif /* _RAISONANCE_ */
typedef enum
{
  COM1 = 0
} COM_TypeDef;

#if 0
#define COMn                        1
#define EVAL_COM1                   USART3
#define EVAL_COM1_GPIO              GPIOG
#define EVAL_COM1_CLK               CLK_Peripheral_USART3
#define EVAL_COM1_RxPin             GPIO_Pin_0
#define EVAL_COM1_TxPin             GPIO_Pin_1
#else
#define COMn                        1
#define EVAL_COM1                   USART2
#define EVAL_COM1_GPIO              GPIOH
#define EVAL_COM1_CLK               CLK_Peripheral_USART2
#define EVAL_COM1_RxPin             GPIO_Pin_4
#define EVAL_COM1_TxPin             GPIO_Pin_5
#endif

USART_TypeDef* COM_USART[COMn] =
  {
    EVAL_COM1
  };

GPIO_TypeDef* COM_PORT[COMn] =
  {
    EVAL_COM1_GPIO
  };
const CLK_Peripheral_TypeDef COM_USART_CLK[COMn] =
  {
    EVAL_COM1_CLK
  };
const uint8_t COM_TX_PIN[COMn] =
  {
    EVAL_COM1_TxPin
  };
const uint8_t COM_RX_PIN[COMn] =
  {
    EVAL_COM1_RxPin
  };

extern bool WakeUpTimeInitialized;
extern volatile uint32_t McuWakeUpTime;


static void BoardUnusedIoInit( void );
static void SystemClockConfig( void );
static void CalibrateSystemWakeupTime( void );
static void SystemClockReConfig( void );

void COMInit(COM_TypeDef COM, uint32_t USART_BaudRate,
                      USART_WordLength_TypeDef USART_WordLength,
                      USART_StopBits_TypeDef USART_StopBits,
                      USART_Parity_TypeDef USART_Parity,
                      USART_Mode_TypeDef USART_Mode)
{
    CLK_PeripheralClockConfig(COM_USART_CLK[COM], ENABLE);
    GPIO_ExternalPullUpConfig(COM_PORT[COM], COM_TX_PIN[COM], ENABLE);
    GPIO_ExternalPullUpConfig(COM_PORT[COM], COM_RX_PIN[COM], ENABLE);
    USART_Init(COM_USART[COM], USART_BaudRate,
         USART_WordLength,
         USART_StopBits,
         USART_Parity,
         USART_Mode);
}

void COMDeInit(COM_TypeDef COM)
{
    USART_Cmd(COM_USART[COM], DISABLE);
    USART_DeInit(COM_USART[COM]);
    CLK_PeripheralClockConfig(COM_USART_CLK[COM], DISABLE);    
}

/**
  * @brief Retargets the C library printf function to the USART.
  * @param[in] c Character to send
  * @retval char Character sent
  * @par Required preconditions:
  * - None
  */
PUTCHAR_PROTOTYPE
{
  /* Write a character to the USART */
  USART_SendData8(EVAL_COM1, c);
  /* Loop until the end of transmission */
  while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_TC) == RESET);

  return (c);
}
/**
  * @brief Retargets the C library scanf function to the USART.
  * @param[in] None
  * @retval char Character to Read
  * @par Required preconditions:
  * - None
  */
GETCHAR_PROTOTYPE
{
  int c = 0;
  /* Loop until the Read data register flag is SET */
  while (USART_GetFlagStatus(EVAL_COM1, USART_FLAG_RXNE) == RESET);
    c = USART_ReceiveData8(EVAL_COM1);
    return (c);
}

void BoardInitPeriph( void )
{
    
}

void BoardInitMcu( void )
{
  CFG->GCR |= CFG_GCR_SWD;
  SystemClockConfig( );

  BoardUnusedIoInit( );
	CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
	COMInit(COM1, (uint32_t)115200, USART_WordLength_8b, USART_StopBits_1,
										USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
										
}

static void BoardUnusedIoInit( void )
{
    GPIO_Init(GPIOA, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOB, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOC, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOD, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOE, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOF, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOG, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOH, GPIO_Pin_All, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(GPIOI, GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3, GPIO_Mode_In_PU_No_IT);
}

void SystemClockConfig( void )
{
	CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);
    
#if 1    
    /* Enable LSE */
    CLK_LSEConfig(CLK_LSE_ON);
    /* Wait for LSE clock to be ready */
    while (CLK_GetFlagStatus(CLK_FLAG_LSERDY) == RESET);
    
    /* Select LSE (32.768 KHz) as RTC clock source */
    CLK_RTCClockConfig(CLK_RTCCLKSource_LSE, CLK_RTCCLKDiv_1);
#else    
    CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_1);
#endif    
    
    CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);   
}

void main(void)
{
	uint8_t ptrArray[8] = {'0','1','2','3','4','5','6','7'};
	
	BoardInitPeriph();
	
  LCD_GLASS_Init();
  //LCD_ContrastConfig(LCD_Contrast_3V0);

  LCD_GLASS_DisplayString(ptrArray);
  while (1)
	{
	}
}

#ifdef  USE_FULL_ASSERT
  /**
    * @brief  Reports the name of the source file and the source line number
    *   where the assert_param error has occurred.
    * @param  file: pointer to the source file name
    * @param  line: assert_param error line source number
    * @retval None
    */
  void assert_failed(uint8_t* file, uint32_t line)
  {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
  }
#endif
/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
