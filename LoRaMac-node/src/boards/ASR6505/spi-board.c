/*!
 * \file      spi-board.c
 *
 * \brief     Target board SPI driver implementation
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
#include "stm8l15x_syscfg.h" 
#include "stm8l15x.h"
#include "utilities.h"
#include "board.h"
#include "gpio.h"
#include "sx126x-board.h"
#include "board-config.h"
#include "spi-board.h"

GPIO_TypeDef* spi_port, *nss_port;
uint16_t mosiIndex;
uint16_t misoIndex;
uint16_t sclkIndex;
uint16_t nssIndex;
static bool SpiInitialized = false;

void SpiInit( Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso, PinNames sclk, PinNames nss )
{
    SPI_TypeDef *spix = SPI1;
    if(SpiInitialized)
        return;
    
    BoardDisableIrq( );

    obj->SpiId = spiId;
    if(spiId == SPI_1) {
        CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, ENABLE);
        spix = SPI1;
    }else if(spiId == SPI_2) {
        CLK_PeripheralClockConfig(CLK_Peripheral_SPI2, ENABLE);
        spix = SPI2;
    }

    {
        mosiIndex = ( 0x01 << ( mosi & 0x0F ) );
        misoIndex = ( 0x01 << ( miso & 0x0F ) );
        sclkIndex = ( 0x01 << ( sclk & 0x0F ) );
        nssIndex = ( 0x01 << ( nss & 0x0F ) );
        if( ( mosi & 0xF0 ) == 0x00 )
        {
            spi_port = GPIOA;
        }
        else if( ( mosi & 0xF0 ) == 0x10 )
        {
            spi_port = GPIOB;
        }
        else if( ( mosi & 0xF0 ) == 0x20 )
        {
            spi_port = GPIOC;
        }
        else if( ( mosi & 0xF0 ) == 0x30 )
        {
            spi_port = GPIOD;
        }
        else if( ( mosi & 0xF0 ) == 0x40 )
        {
            spi_port = GPIOE;
        } 
        else if( ( mosi & 0xF0 ) == 0x50 )
        {
            spi_port = GPIOF;
        } 
        else if( ( mosi & 0xF0 ) == 0x60 )
        {
            spi_port = GPIOG;
        }
        else if( ( mosi & 0xF0 ) == 0x70 )
        {
            spi_port = GPIOH;
        }
        else if( ( mosi & 0xF0 ) == 0x80 )
        {
            spi_port = GPIOI;
        }
        obj->Mosi.port = spi_port;
        obj->Miso.port = spi_port;
        obj->Sclk.port = spi_port;

        obj->Mosi.pinIndex = mosiIndex;
        obj->Miso.pinIndex = misoIndex;
        obj->Sclk.pinIndex = sclkIndex;

        GPIO_ExternalPullUpConfig(spi_port, mosiIndex | misoIndex | sclkIndex, ENABLE);
        
        /* pin remap */
        if(spix==SPI2 && sclk==PI_1)
            SYSCFG_REMAPPinConfig(REMAP_Pin_SPI2Full, ENABLE);
        else if(spix==SPI1 && sclk==PF_2)
            SYSCFG_REMAPPinConfig(REMAP_Pin_SPI1PortF, ENABLE);                
        
        SPI_Init(spix, SPI_FirstBit_MSB, SPI_BaudRatePrescaler_32, SPI_Mode_Master,
           SPI_CPOL_Low, SPI_CPHA_1Edge, SPI_Direction_2Lines_FullDuplex,
           SPI_NSS_Soft, 0x07);
        SPI_Cmd(spix, ENABLE);
    }
    
    SpiInitialized = true;
    BoardEnableIrq( );
}

void SpiDeInit( Spi_t *obj )
{
    if(!spi_port || !SpiInitialized)
        return;
    
    if(obj->SpiId == SPI_1) {
        SPI_Cmd(SPI1, DISABLE);
        SPI_DeInit(SPI1);
        CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, DISABLE);
    }else if(obj->SpiId == SPI_2) {
        SPI_Cmd(SPI2, DISABLE);
        SPI_DeInit(SPI2);
        CLK_PeripheralClockConfig(CLK_Peripheral_SPI2, DISABLE);
    }    
 
    GPIO_ExternalPullUpConfig(spi_port, mosiIndex | misoIndex | sclkIndex, DISABLE);
    GPIO_Init(obj->Nss.port, obj->Nss.pinIndex, GPIO_Mode_Out_PP_High_Fast);
    
    GPIO_Init(spi_port, sclkIndex, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(spi_port, misoIndex, GPIO_Mode_In_PU_No_IT);
    GPIO_Init(spi_port, mosiIndex, GPIO_Mode_In_PU_No_IT);  
   
    SpiInitialized = false;
}

void SpiFormat( Spi_t *obj, int8_t bits, int8_t cpol, int8_t cpha, int8_t slave )
{
    
}

void SpiFrequency( Spi_t *obj, uint32_t hz )
{
    
}

uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    SPI_TypeDef *spix = obj->SpiId==SPI_1?SPI1:SPI2;
    uint8_t rxData = 0;

    BoardDisableIrq( );

    /* Loop while DR register in not emplty */
    while (SPI_GetFlagStatus(spix, SPI_FLAG_TXE) == RESET);
    
    /* Send byte through the SPI peripheral */
    SPI_SendData(spix, outData);
    
    /* Wait to receive a byte */
    while (SPI_GetFlagStatus(spix, SPI_FLAG_RXNE) == RESET);
    
    /* Return the byte read from the SPI bus */
    rxData = SPI_ReceiveData(spix);

    BoardEnableIrq( );
    return( rxData );
}

