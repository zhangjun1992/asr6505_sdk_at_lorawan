/*!
 * \file      board.c
 *
 * \brief     Target board general functions implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include <stdio.h> 
#include "stm8l15x.h"
#include "stm8l15x_rtc.h"
#include "stm8l15x_flash.h"
#include "utilities.h"
#include "gpio.h"
#include "spi.h"
#include "board-config.h"
#include "timer.h"
#include "rtc-board.h"
#include "sx126x-board.h"
#include "board.h"
#include "radio.h"
#include "stm8l15x_pwr.h"

//uart
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
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

volatile uint8_t HasLoopedThroughMainNum = 0;
static bool UartLowPowerDisableDuringTask = true;
static volatile bool McuStopFlag = false;
static RadioEvents_t TestRadioEvents;


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
    USART_Cmd(COM_USART[COM], false);
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

static TimerEvent_t CalibrateSystemWakeupTimeTimer;
static bool McuInitialized = false;
static bool SystemWakeupTimeCalibrated = false;

static void OnCalibrateSystemWakeupTimeTimerEvent( void )
{
    SystemWakeupTimeCalibrated = true;
}

uint8_t GetBoardPowerSource(void)
{
	return BATTERY_POWER;
}

void BoardDisableIrq( void )
{
    disableInterrupts( );
}

void BoardEnableIrq( void )
{
    enableInterrupts( );
}

void BoardInitPeriph( void )
{
    
}

void BoardInitMcu( void )
{  
    if( McuInitialized == false )
    {
        CFG->GCR |= CFG_GCR_SWD;
        SystemClockConfig( );
        
        BoardUnusedIoInit( );

        RtcInit( );
    }
    else
    {
        SystemClockReConfig( );
    }
    
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, ENABLE);
    COMInit(COM1, (uint32_t)115200, USART_WordLength_8b, USART_StopBits_1,
                       USART_Parity_No, (USART_Mode_TypeDef)(USART_Mode_Tx | USART_Mode_Rx));
#ifdef LORA_AT_COMMAND
    USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
#endif
    
    SX126xIoInit( );    
    SpiInit(&SX126x.Spi, RADIO_SPI_ID, RADIO_MOSI, RADIO_MISO, RADIO_SCLK, NC/*RADIO_NSS*/);  
    
    if( McuInitialized == false )
    {
        McuInitialized = true;        
        //read the last MCU wakeup time
        WakeUpTimeInitialized = (bool)(*(uint8_t *)FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS);
        McuWakeUpTime = (uint32_t)(*(uint8_t *)(FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS + sizeof(uint8_t)));
        
        if(!WakeUpTimeInitialized) {
            if( GetBoardPowerSource( ) == BATTERY_POWER )
            {
                CalibrateSystemWakeupTime( );
            }
            
            FLASH_SetProgrammingTime(FLASH_ProgramTime_Standard);
            FLASH_Unlock(FLASH_MemType_Data);
            //FLASH_ProgramOptionByte(0x4800,0x11); //ROP on
            //FLASH_ProgramOptionByte(0x480B,0x55); //bootloader enable
            //FLASH_ProgramOptionByte(0x480C,0xAA); //bootloader enable
            
            FLASH_ProgramByte(FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS, (uint8_t)WakeUpTimeInitialized);
            FLASH_ProgramByte(FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS + sizeof(uint8_t), (uint8_t)McuWakeUpTime);
            FLASH_Lock(FLASH_MemType_Data);
        }        
    }    
}

void BoardResetMcu( void )
{
    BoardDisableIrq( );
}

void BoardDeInitMcu( void )
{
    COMDeInit(COM1);
    SpiDeInit( &SX126x.Spi );
    SX126xIoDeInit( );
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM4, DISABLE);
}

uint32_t BoardGetRandomSeed( void )
{
    return 0;
}

#define         ID1                                 ( 0x4926 )
#define         ID2                                 ( 0x492A )
#define         ID3                                 ( 0x492E )

void BoardGetUniqueId( uint8_t *id )
{
    id[7] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 24;
    id[6] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 16;
    id[5] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) ) >> 8;
    id[4] = ( ( *( uint32_t* )ID1 )+ ( *( uint32_t* )ID3 ) );
    id[3] = ( ( *( uint32_t* )ID2 ) ) >> 24;
    id[2] = ( ( *( uint32_t* )ID2 ) ) >> 16;
    id[1] = ( ( *( uint32_t* )ID2 ) ) >> 8;
    id[0] = ( ( *( uint32_t* )ID2 ) );
}

uint16_t BoardBatteryMeasureVolage( void )
{
    return 0;
}

uint32_t BoardGetBatteryVoltage( void )
{
    return 0;
}

uint8_t BoardGetBatteryLevel( void )
{
    return 0;
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

void CalibrateSystemWakeupTime( void )
{
    if( SystemWakeupTimeCalibrated == false )
    {
        TimerInit( &CalibrateSystemWakeupTimeTimer, OnCalibrateSystemWakeupTimeTimerEvent );
        TimerSetValue( &CalibrateSystemWakeupTimeTimer, 1000 );
        TimerStart( &CalibrateSystemWakeupTimeTimer );
        while( SystemWakeupTimeCalibrated == false )
        {
            TimerLowPowerHandler( );
        }
    }
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


void SystemClockReConfig( void )
{
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1);   
}

void UartEnterLowPowerStopMode( void )
{  
    if (Radio.GetStatus() != RF_IDLE) {
        return;
    }
    
    if( UartLowPowerDisableDuringTask == false )
    {
        Radio.Init( &TestRadioEvents );
        Radio.Sleep();

        BoardDeInitMcu( );
        GPIO_Init(GPIOG, GPIO_Pin_0, GPIO_Mode_In_PU_IT);
        EXTI_SelectPort(EXTI_Port_G);
        EXTI_SetHalfPortSelection(EXTI_HalfPort_G_LSB, ENABLE);
        EXTI_SetPortSensitivity(EXTI_Port_G, EXTI_Trigger_Falling);
        PWR_UltraLowPowerCmd(ENABLE);  

        McuStopFlag = true;
        halt();
    }   
}

void UartLowPowerHandler( void )
{
    if( HasLoopedThroughMainNum < 5 )
		{
				HasLoopedThroughMainNum++;
		}
	  else
		{
				HasLoopedThroughMainNum = 0;
				if( GetBoardPowerSource( ) == BATTERY_POWER )
				{
						UartEnterLowPowerStopMode( );
				}
		}
}

void UartRecoverMcuStatus( void )
{
    if(McuStopFlag) {
        BoardInitMcu( );
        McuStopFlag = false;
    }
}

void DisableLowPowerDuringTask ( bool status )
{  
    if( status == true )
    {
        UartRecoverMcuStatus( );
    }   
    UartLowPowerDisableDuringTask = status;   
}
