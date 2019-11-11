/*!
 * \file      board-config.h
 *
 * \brief     Board configuration
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
 *               ___ _____ _   ___ _  _____ ___  ___  ___ ___
 *              / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
 *              \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
 *              |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
 *              embedded.connectivity.solutions===============
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 *
 * \author    Daniel Jaeckle ( STACKFORCE )
 *
 * \author    Johannes Bruder ( STACKFORCE )
 */
#ifndef __BOARD_CONFIG_H__
#define __BOARD_CONFIG_H__

/*!
 * Defines the time required for the TCXO to wakeup [ms].
 */
#define BOARD_TCXO_WAKEUP_TIME                      5

/*!
 * Board MCU pins definitions
 */
#if 0 
#define RADIO_SPI_ID                                SPI_1

#define RADIO_RESET                                 PC_7

#define RADIO_MOSI                                  PB_6
#define RADIO_MISO                                  PB_7
#define RADIO_SCLK                                  PB_5

#define RADIO_NSS                                   PD_1

#define RADIO_BUSY                                  PE_1
#define RADIO_DIO_1                                 PC_2
#define RADIO_ANT_SWITCH_POWER                      PD_3

#else
#define RADIO_SPI_ID                                SPI_2

#define RADIO_RESET                                 PH_0

#define RADIO_MOSI                                  PI_2
#define RADIO_MISO                                  PI_3
#define RADIO_SCLK                                  PI_1

#define RADIO_NSS                                   PD_7

#define RADIO_BUSY                                  PH_1
#define RADIO_DIO_1                                 PH_2
#define RADIO_ANT_SWITCH_POWER                      PG_6

#endif

#endif // __BOARD_CONFIG_H__
