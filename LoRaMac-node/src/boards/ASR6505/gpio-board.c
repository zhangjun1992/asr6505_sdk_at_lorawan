/*!
 * \file      gpio-board.c
 *
 * \brief     Target board GPIO driver implementation
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
#include <stddef.h>
#include "stm8l15x.h"
#include "stm8l15x_exti.h"
#include "stm8l15x_gpio.h"
#include "utilities.h"
#include "board-config.h"
#include "timer.h"
#include "rtc-board.h"
#include "gpio-board.h"

static GpioIrqHandler *GpioIrq[16];

void HAL_GPIO_EXTI_Callback( uint16_t gpioPin );

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value )
{ 
    if( pin < IOE_0 ) {
        GPIO_Mode_TypeDef gpio_mode = GPIO_Mode_In_PU_No_IT;
        obj->pin = pin;
        obj->pull = type;
        if( pin == NC ) {
            return;
        }

        obj->pinIndex = ( 0x01 << ( obj->pin & 0x0F ) );
        switch(obj->pin & 0xF0) {
            case 0x00: obj->port = GPIOA; break;
            case 0x10: obj->port = GPIOB; break;
            case 0x20: obj->port = GPIOC; break;
            case 0x30: obj->port = GPIOD; break;
            case 0x40: obj->port = GPIOE; break;
            case 0x50: obj->port = GPIOF; break;
            case 0x60: obj->port = GPIOG; break;
            case 0x70: obj->port = GPIOH; break;
            case 0x80: obj->port = GPIOI; break;
            default: break;
        }
        
        switch(mode){
            case PIN_INPUT: {
                gpio_mode = (type==PIN_PULL_UP)?GPIO_Mode_In_PU_No_IT:GPIO_Mode_In_FL_No_IT;
                break;
            }
            case PIN_OUTPUT: {
                if(config==PIN_OPEN_DRAIN)
                    gpio_mode = (value==0)?GPIO_Mode_Out_OD_Low_Slow:GPIO_Mode_Out_OD_HiZ_Slow;
                else
                    gpio_mode = (value==0)?GPIO_Mode_Out_PP_Low_Slow:GPIO_Mode_Out_PP_High_Slow;
                break;
            }
            default: break;
        }

        GPIO_Init(obj->port, obj->pinIndex, gpio_mode);
        if( mode == PIN_OUTPUT )
            GpioMcuWrite( obj, value );
    }

}

uint16_t GpioMcuExtiPinGet(uint16_t pinIndex)
{
    EXTI_Pin_TypeDef extiPin;
    switch( pinIndex ) {
        case GPIO_Pin_0: extiPin = EXTI_Pin_0; break;
        case GPIO_Pin_1: extiPin = EXTI_Pin_1; break;
        case GPIO_Pin_2: extiPin = EXTI_Pin_2; break;
        case GPIO_Pin_3: extiPin = EXTI_Pin_3; break;
        case GPIO_Pin_4: extiPin = EXTI_Pin_4; break;
        case GPIO_Pin_5: extiPin = EXTI_Pin_5; break;
        case GPIO_Pin_6: extiPin = EXTI_Pin_6; break;
        case GPIO_Pin_7: extiPin = EXTI_Pin_7; break;
        default: break;
    }
    
    return extiPin;
}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler )
{
    if( obj->pin < IOE_0 )
    {
        GPIO_Mode_TypeDef gpio_mode = GPIO_Mode_In_PU_IT;
        EXTI_Trigger_TypeDef intr_type = EXTI_Trigger_Rising;

        if( !irqHandler )
        {
            return;
        }
        
        if( irqMode == IRQ_RISING_EDGE )
        {
            intr_type = EXTI_Trigger_Rising;
        }
        else if( irqMode == IRQ_FALLING_EDGE )
        {
            intr_type = EXTI_Trigger_Falling;
        }
        else
        {
            intr_type = EXTI_Trigger_Rising_Falling;
        }
        disableInterrupts();
        GPIO_Init(obj->port, obj->pinIndex, gpio_mode);
        EXTI_SetPinSensitivity((EXTI_Pin_TypeDef)GpioMcuExtiPinGet(obj->pinIndex), intr_type);
        GpioIrq[( obj->pin ) & 0x0F] = irqHandler;
        enableInterrupts();
    }
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        GPIO_Init(obj->port, obj->pinIndex, GPIO_Mode_In_PU_No_IT);

        GpioIrq[( obj->pin ) & 0x0F] = NULL;
    }
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
    if( obj->pin < IOE_0 )
    {
        if( ( obj == NULL ) || ( obj->port == NULL ) )
        {
            assert_param( FAIL );
        }
        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return;
        }
        GPIO_WriteBit( obj->port, (GPIO_Pin_TypeDef)obj->pinIndex, (BitAction)value );
    }
}

void GpioMcuToggle( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        if( ( obj == NULL ) || ( obj->port == NULL ) )
        {
            assert_param( FAIL );
        }

        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return;
        }
        GPIO_ToggleBits( obj->port, obj->pinIndex );
    }
}

uint32_t GpioMcuRead( Gpio_t *obj )
{
    if( obj->pin < IOE_0 )
    {
        if( obj == NULL )
        {
            assert_param( FAIL );
        }
        // Check if pin is not connected
        if( obj->pin == NC )
        {
            return 0;
        }
        return GPIO_ReadInputDataBit( obj->port, (GPIO_Pin_TypeDef)obj->pinIndex );
    }
    
    return 0;
}

void HAL_GPIO_EXTI_IRQHandler(EXTI_IT_TypeDef EXTI_IT)
{
#if !defined( USE_NO_TIMER )
    RtcRecoverMcuStatus( );
#endif
    
    if (EXTI_GetITStatus(EXTI_IT) != RESET) {
      
        //clear the it status first, otherwise when do call RadioIrqProcess when it enableinterrupts() it will re-enter into the interrupt handler
        EXTI_ClearITPendingBit(EXTI_IT);
        HAL_GPIO_EXTI_Callback(EXTI_IT);
    }
}

void HAL_GPIO_EXTI_Callback( uint16_t gpioPin )
{
    uint8_t callbackIndex = 0;

    if( gpioPin > 0 )
    {
        while( gpioPin != 0x01 )
        {
            gpioPin = gpioPin >> 1;
            callbackIndex++;
        }
    }

    if( GpioIrq[callbackIndex] != NULL )
    {
        GpioIrq[callbackIndex]( );
    }
}
