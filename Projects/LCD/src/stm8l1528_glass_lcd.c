/**
  ******************************************************************************
  * @file    stm8l1528_eval_glass_lcd.c
  * @author  MCD Application Team
  * @version V2.1.0
  * @date    09/24/2010
  * @brief   This file includes driver for the glass LCD Module mounted on
  *          EVAL board.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm8l1528_glass_lcd.h"
#include "stdio.h"

/** @addtogroup Utilities
  * @{
  */

/** @defgroup STM8L1528_EVAL_GLASS_LCD
  * @brief This file includes the LCD Glass driver for Pacific Display
  *        (Custom LCD 8x40) Module of Big-Falco-EVAL board.
  * @{
  */

/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Types
  * @{
  */
/**
  * @}
  */


/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Variables
  * @{
  */

/* LCD connect to chip */
/*********************
LCD pin      Chip pin
COM1         LCD_COM0
COM2         LCD_COM1
COM3         LCD_COM2
COM4         LCD_COM3
2            LCD_SEG4
3            LCD_SEG5
4            LCD_SEG6
5            LCD_SEG7
6            LCD_SEG8
7            LCD_SEG9
8            LCD_SEG14
9            LCD_SEG15
10           LCD_SEG16
11           LCD_SEG17
12           LCD_SEG18
13           LCD_SEG19
14           LCD_SEG36
15           LCD_SEG37
16           LCD_SEG38
17           LCD_SEG39
***********************/

/**
  @verbatim
================================================================================
                              GLASS LCD MAPPING
================================================================================
						A
          ------
          |    |
         F|    |B
          |    |
          --G-- 
          |    |
         E|    |C
          |    |
          ------
						D

0 LCD number coding is based on the following matrix:
             COM0    COM1    COM2    COM3
  SEG(n)    { D ,     E ,     G ,     F }
  SEG(n+1)  { 0 ,     C ,     B ,     A }

The number 0 for example is:
-----------------------------------------------------------
             COM0    COM1    COM2    COM3
  SEG(n)    { 1 ,     1 ,     0 ,     1 }
  SEG(n+1)  { 0 ,     1 ,     1 ,     1 }
   --------------------------------------------------------
           =  1       3       2       3 hex

   => '0' = 0x1323

  @endverbatim
  */

/**
  * @brief LETTERS AND NUMBERS MAPPING DEFINITION
  */
uint16_t i;
const uint32_t mask [] =
  {
    0xF000, 0x0F00, 0x00F0, 0x000F
  };
const uint8_t shift[4] =
  {
		12, 8, 4, 0
  };

uint8_t digit[4];     /* Digit frame buffer */

/* number map of the custom LCD */
__CONST uint32_t NumberMap[10] =
  {
    /*  0       1       2       3       4   */
    0x1323, 0x0220, 0x1132, 0x1232, 0x0231,
    /*  5       6       7       8       9   */
    0x1213, 0x1313, 0x0222, 0x1333, 0x1233
  };

/**
  * @}
  */


/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Function_Prototypes
  * @{
  */
static void Convert(uint8_t* c);
static void delay(__IO uint32_t nCount);

/**
  * @}
  */

/** @defgroup STM8L1528_EVAL_GLASS_LCD_Private_Functions
  * @{
  */


/*  * @brief  Configures the LCD GLASS GPIO port IOs and LCD peripheral.
  * @param  None
  * @retval None
  */

void LCD_GLASS_Init(void)
{
  /*
    The LCD is configured as follow:
     - clock source = LSE (32.768 KHz)
     - Voltage source = Internal
     - Prescaler = 2
     - Divider = 18 (16 + 2)  
     - Mode = 1/8 Duty, 1/4 Bias
     - LCD frequency = (clock source * Duty) / (Prescaler * Divider)
                     = 114 Hz ==> Frame frequency = 28,5 Hz*/

  /* Enable LCD clock */
  CLK_PeripheralClockConfig(CLK_Peripheral_LCD, ENABLE);
  CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_1);
  LCD_Init(LCD_Prescaler_1, LCD_Divider_31, LCD_Duty_1_4, 
                                    LCD_Bias_1_3, LCD_VoltageSource_Internal);

  LCD_PortMaskConfig(LCD_PortMaskRegister_0, 0xFF);
  LCD_PortMaskConfig(LCD_PortMaskRegister_1, 0xFF);
  LCD_PortMaskConfig(LCD_PortMaskRegister_2, 0xFF);
  LCD_PortMaskConfig(LCD_PortMaskRegister_3, 0xFF);
  LCD_PortMaskConfig(LCD_PortMaskRegister_4, 0xFF);
  LCD_PortMaskConfig(LCD_PortMaskRegister_5, 0xFF);

  LCD_ContrastConfig(LCD_Contrast_3V0);
  LCD_PulseOnDurationConfig(LCD_PulseOnDuration_1);
  LCD_Cmd(ENABLE); /*!< Enable LCD peripheral */
}

/**
  * @brief  This function writes a char on the LCD Glass.
  * @param  ch: the character to dispaly.
  * @param  position: position in the LCD of the character to write [0:7]
  * @retval None
  */
void LCD_GLASS_WriteChar(uint8_t* ch, uint8_t Position)
{
	Convert(ch);
//digit[0] = 0xF;
//digit[1] = 0xE;
//digit[2] = 0x0;
//digit[3] = 0x0;
  /* Enable the write access on the LCD RAM first banck */
  LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;
  switch (Position)
  {
      /* Position 0 on LCD (Digit1)*/
    case 0:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_0] &= 0x0ef;
      LCD->RAM[LCD_RAMRegister_0] |= (uint8_t)((digit[0]<<4)& 0x10);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0fc;
      LCD->RAM[LCD_RAMRegister_4] |= (uint8_t)(digit[1]&0x03);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_7] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_7] |= (uint8_t)((digit[2]<<4)&0x30);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0xfc;
      LCD->RAM[LCD_RAMRegister_11] |= (uint8_t)(digit[3]& 0x03);
      break;

      /* Position 1 on LCD */
    case 1:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_0] &= 0x0bf;
      LCD->RAM[LCD_RAMRegister_0] |= (uint8_t)((digit[0]<<6)&0x40);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0f3;
      LCD->RAM[LCD_RAMRegister_4] |= (uint8_t)((digit[1]<<2)&0x0c);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_7] &= 0x03f;
      LCD->RAM[LCD_RAMRegister_7] |= (uint8_t)((digit[2]<<6)&0xc0);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0xf3;
      LCD->RAM[LCD_RAMRegister_11] |= (uint8_t)((digit[3]<<2)&0x0c);
      break;

      /* Position 2 on LCD (Digit3)*/
    case 2:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_1] &= 0x0fe;
      LCD->RAM[LCD_RAMRegister_1] |= (uint8_t)(digit[0]&0x01);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_4] |= (uint8_t)((digit[1]<<4)&0x30);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_8] &= 0x0fc;
      LCD->RAM[LCD_RAMRegister_8] |= (uint8_t)(digit[2]&0x03);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_11] |= (uint8_t)((digit[3]<<4)&0x30);
      break;

      /* Position 3 on LCD */
    case 3:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_1] &= 0x0bf;
      LCD->RAM[LCD_RAMRegister_1] |= (uint8_t)((digit[0]<<6)&0x40);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x0f3;
      LCD->RAM[LCD_RAMRegister_5] |= (uint8_t)((digit[1]<<2)&0x0c);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_8] &= 0x03f;
      LCD->RAM[LCD_RAMRegister_8] |= (uint8_t)((digit[2]<<6)&0xc0);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0xf3;
      LCD->RAM[LCD_RAMRegister_12] |= (uint8_t)((digit[3]<<2)&0x0c);
      break;

      /* Position 4 on LCD (Digit5)*/
    case 4:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_2] &= 0x0fe;
      LCD->RAM[LCD_RAMRegister_2] |= (uint8_t)(digit[0]&0x01);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_5] |= (uint8_t)((digit[1]<<4)&0x30);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_9] &= 0x0fc;
      LCD->RAM[LCD_RAMRegister_9] |= (uint8_t)(digit[2]&0x03);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_12] |= (uint8_t)((digit[3]<<4)&0x30);
      break;

      /* Position 5 on LCD (Digit6)*/
    case 5:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_2] &= 0x0fb;
      LCD->RAM[LCD_RAMRegister_2] |= (uint8_t)((digit[0]<<2)&0x04);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x03f;
      LCD->RAM[LCD_RAMRegister_5] |= (uint8_t)((digit[1]<<6)&0xc0);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_9] &= 0x0f3;
      LCD->RAM[LCD_RAMRegister_9] |= (uint8_t)((digit[2]<<2)&0x0c);	
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0x03f;
      LCD->RAM[LCD_RAMRegister_12] |= (uint8_t)((digit[3]<<6)&0xc0);
      break;

      /* Position 6 on LCD (Digit7)*/
    case 6:

      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_15] &= 0x0ef;
      LCD->RAM[LCD_RAMRegister_15] |= (uint8_t)((digit[0]<<4)&0x10);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_17] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_17] |= (uint8_t)((digit[1]<<4)&0x30);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_19] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_19] |= (uint8_t)((digit[2]<<4)&0x30);
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_21] &= 0x0cf;
      LCD->RAM[LCD_RAMRegister_21] |= (uint8_t)((digit[3]<<4)&0x30);
      break;

      /* Position 7 on LCD (Digit8)*/
    case 7:

      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_15] &= 0x0bf;
      LCD->RAM[LCD_RAMRegister_15] |= (uint8_t)((digit[0]<<6)&0x40);
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_17] &= 0x3f;
      LCD->RAM[LCD_RAMRegister_17] |= (uint8_t)((digit[1]<<6)&0x0c0);
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_19] &= 0x3f;
      LCD->RAM[LCD_RAMRegister_19] |= (uint8_t)((digit[2]<<6)&0x0c0);
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_21] &= 0x3f;
      LCD->RAM[LCD_RAMRegister_21] |= (uint8_t)((digit[3]<<6)&0x0c0);
      break;

    default:
      break;
  }
}

/**
  * @brief  This function Clear a character on the LCD Glass.
  * @param  position: position in the LCD of the character to Clear [0:7]
  * @retval None
  */
void LCD_GLASS_ClearChar(uint8_t Position)
{
  switch (Position)
  {
      /* Position 0 on LCD (Digit1)*/
    case 0:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_0] &= 0x0ef;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0fc;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_7] &= 0x0cf;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0xfc;
      break;

      /* Position 1 on LCD */
    case 1:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_0] &= 0x0bf;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0f3;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_7] &= 0x03f;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0xf3;
      break;

      /* Position 2 on LCD (Digit3)*/
    case 2:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_1] &= 0x0fe;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_4] &= 0x0cf;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_8] &= 0x0fc;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_11] &= 0x0cf;
      break;

      /* Position 3 on LCD */
    case 3:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_1] &= 0x0bf;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x0f3;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_8] &= 0x03f;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0xf3;
      break;

      /* Position 4 on LCD (Digit5)*/
    case 4:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_2] &= 0x0fe;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x0cf;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_9] &= 0x0fc;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0x0cf;
      break;

      /* Position 5 on LCD (Digit6)*/
    case 5:
      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_2] &= 0x0fb;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_5] &= 0x03f;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_9] &= 0x0f3;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_12] &= 0x03f;
      break;

      /* Position 6 on LCD (Digit7)*/
    case 6:

      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_15] &= 0x0fe;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_17] &= 0x0fc;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_19] &= 0x0fc;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_21] &= 0x0fc;
      break;

      /* Position 7 on LCD (Digit8)*/
    case 7:

      /* Enable the write access on the LCD RAM First banck */
      LCD->CR4 &= (uint8_t)~LCD_CR4_PAGECOM;

      /* COM0 */
      LCD->RAM[LCD_RAMRegister_15] &= 0x0fb;
    
      /* COM1 */
      LCD->RAM[LCD_RAMRegister_17] &= 0x0f3;
  
      /* COM2 */			
      LCD->RAM[LCD_RAMRegister_19] &= 0x0f3;
      
      /* COM3 */			
      LCD->RAM[LCD_RAMRegister_21] &= 0x0f3;
      break;

    default:
      break;
  }
}

/**
  * @brief  This function Clears the LCD display memory.
  * @param  None
  * @retval None
  */
void LCD_GLASS_Clear(void)
{
  uint8_t counter = 0x00;

  /* Enable the write access on the LCD RAM First banck */
  LCD->CR4 &= (uint8_t)(~LCD_CR4_PAGECOM);

  for (counter = 0x0; counter < 0x16; counter++)
  {
    LCD->RAM[counter] =  LCD_RAM_RESET_VALUE;
  }

}

/**
  * @brief  Writes a String on the LCD Glass.
  * @param  ptr: Pointer to the string to display on the LCD Glass.
  * @retval None
  */
void LCD_GLASS_DisplayString(uint8_t* ptr)
{
  uint8_t i = 0x00;

  /* Send the string character by character on lCD */
  while ((*ptr != 0) & (i < 8))
  {
    /* Display one character on LCD */
    LCD_GLASS_WriteChar(ptr, i);

    /* Point to the next character */
    ptr++;

    /* Increment the character counter */
    i++;
  }
}

/**
  * @brief  Converts an ascii char to the a LCD digit (previous coding).
  * @param  c: a char to display.
  * @retval None
  */
static void Convert(uint8_t* c)
{
  uint32_t ch = 0 , tmp = 0;
  uint16_t i;

  /* The character c is a number*/
  if ((*c < (uint8_t)0x3A) && (*c > (uint8_t)0x2F))
  {
    ch = NumberMap[*c-(uint8_t)0x30];
  }

  for (i = 0;i < 4; i++)
  {
    tmp = ch & mask[i];
    digit[i] = (uint8_t)(tmp >> (uint8_t)shift[i]);
  }
}
/**
  * @brief  Inserts a delay time.
  * @param  nCount: specifies the delay time length.
  * @retval None
  */
static void delay(__IO uint32_t nCount)
{
  __IO uint32_t index = 0;
  for (index = (0x60 * nCount); index != 0; index--)
  {}
}

/**
  * @}
  */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
