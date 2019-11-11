#include <stdio.h>
#include <stdlib.h>
#include "stm8l15x_pwr.h"
#include "radio.h"
#include "sx126x.h"
#include "timer.h"
#include "board.h"
#include "sx126x-board.h"
#include "test_cases.h"
#include "stm8l15x_rtc.h"
#include "stm8l15x_tim1.h"


#define RF_FREQUENCY                    470000000
#define TX_OUTPUT_POWER                             22        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       11         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         5         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define LORA_RX_DELAY  1
#define LORA_TX_DELAY  1000


//global variables
static uint32_t g_fcnt_start = 0;
static uint32_t g_fcnt_rcvd = 0;
static uint32_t g_fcnt_send = 1;
static uint32_t g_freq = RF_FREQUENCY;
static uint8_t g_sf = LORA_SPREADING_FACTOR;
static uint8_t g_tx_power = TX_OUTPUT_POWER;
static uint8_t g_tx_len = 0;
static bool g_lora_test_rxs = false;
static TimerEvent_t LoraTxTestTimer;
static TimerEvent_t WakeupTestTimer;
static RadioEvents_t TestRadioEvents;


void OnTxDone( void )
{
    printf("OnTxDone\r\n");
    TimerSetValue(&LoraTxTestTimer, LORA_TX_DELAY);
    TimerStart(&LoraTxTestTimer);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    int i=0;
    
    printf("OnRxDone\r\n");
    if(g_lora_test_rxs) {
        g_fcnt_rcvd++;
        printf("[%lu]Received, rssi = %d, snr = %d\r\n", g_fcnt_rcvd, rssi, (int16_t)snr);
        printf("  ");
        for(i=0; i<size; i++){
            printf("0x%x ", (void *)payload[i]);
        }
        printf("\r\n");
    }else{
        uint32_t fcnt = strtol((const char *)payload, NULL, 0);
        if(!g_fcnt_start) g_fcnt_start = fcnt;
        g_fcnt_rcvd ++;
        
        if(fcnt) {
            printf("[%lu/%lu]Received: %lu, rssi = %d, snr = %d\r\n", g_fcnt_rcvd, (fcnt-g_fcnt_start+1), fcnt, rssi, (int16_t)snr);
        }else{
            printf("Received, size: %u, rssi = %d, snr = %d\r\n", size, rssi, (int16_t)snr);
            printf("  ");
            for(i=0; i<size; i++){
                printf("0x%x ", (void *)payload[i]);
            }
            printf("\r\n");
        }
    }
    
}

void OnTxTimeout( void )
{
    printf("OnTxTimeout\r\n");
    TimerSetValue(&LoraTxTestTimer, LORA_TX_DELAY);
    TimerStart(&LoraTxTestTimer);
}

void OnRxTimeout( void )
{
    printf("OnRxTimeout\r\n");
}

void OnRxError( void )
{
    printf("OnRxError\r\n");
}


void OnTxTimerEvent( void )
{
    char buf[32];
    uint32_t size = 0;
    
    //stop timer
    TimerStop(&LoraTxTestTimer);
    
    size = sprintf(buf, "%lu", g_fcnt_send);
    if(size) {
        printf("start to tx data(freq: %lu, dr: %u, power: %u): %lu\r\n", g_freq, (uint16_t)12-g_sf, (uint16_t)g_tx_power, g_fcnt_send);
        Radio.SetChannel(g_freq);
        Radio.SetTxConfig( MODEM_LORA, g_tx_power, 0, LORA_BANDWIDTH,
                               g_sf, LORA_CODINGRATE,
                               LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                               true, 0, 0, LORA_IQ_INVERSION_ON, 6000 );

        Radio.Send( (uint8_t *)buf, size>g_tx_len?size:g_tx_len );
        
        g_fcnt_send++;
    }
}


int test_case_ctxcw(int argc, char *argv[])
{
    uint8_t opt = 0;
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t pwr = strtol(argv[1], NULL, 0);
    if(argc>2)
        opt = strtol(argv[2], NULL, 0);
            
    BoardInitMcu();
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;

    SX126xSetPaOpt(opt);
    Radio.Init( &TestRadioEvents );
    
    printf("Start to txcw (freq: %lu, power: %udb, opt: %u)\r\n", freq, (uint16_t)pwr, (uint16_t)opt);
    Radio.SetTxContinuousWave(freq, pwr, 0xffff);
    
    return 0;
}

int test_case_crx(int argc, char *argv[])
{
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr = strtol(argv[1], NULL, 0); 
    
    g_freq = freq;
    g_sf = 12-dr;
    
    BoardInitMcu();
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;

    Radio.Init( &TestRadioEvents );

    Radio.SetChannel(g_freq);
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, g_sf,
                                       LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    g_lora_test_rxs = false;
    Radio.Rx(0);
    
    printf("start to recv package (freq: %lu, dr:%u)\r\n", freq, (uint16_t)dr);

    while(1) {
        Radio.IrqProcess( );
    }
}

int test_case_crxs(int argc, char *argv[])
{
    uint8_t cr = 1;
    uint8_t ldo = 0;
    
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr = strtol(argv[1], NULL, 0);
    if(argc>2)
        cr = strtol(argv[2], NULL, 0);
    if(argc>3)
        ldo = strtol(argv[3], NULL, 0);
    
    g_freq = freq;
    g_sf = 12-dr;
    
    BoardInitMcu();
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;
    Radio.Init( &TestRadioEvents );

    Radio.SetChannel(g_freq);
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, g_sf,
                                       cr, 0, LORA_PREAMBLE_LENGTH,
                                       LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                       0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
    
    SX126x.ModulationParams.Params.LoRa.LowDatarateOptimize = ldo;
    SX126xSetModulationParams( &SX126x.ModulationParams );

    g_lora_test_rxs = true; 
    Radio.Rx(0);
    
    printf("start to recv package (freq: %lu, dr:%u, cr: %u, ldo: %u)\r\n", freq, (uint16_t)dr, (uint16_t)cr, (uint16_t)ldo);
    while(1) {
        Radio.IrqProcess( );
    }
}

int test_case_ctx(int argc, char *argv[])
{
    uint8_t len = 0;
    uint32_t freq = strtol(argv[0], NULL, 0);
    uint8_t dr = strtol(argv[1], NULL, 0);  
    uint8_t pwr = strtol(argv[2], NULL, 0);
    if(argc>3)
        len = strtol(argv[3], NULL, 0);
    
    g_freq = freq;
    g_sf = 12-dr;
    g_tx_power = pwr;
    g_tx_len = len;
    
    BoardInitMcu();
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;
    Radio.Init( &TestRadioEvents );
    
    TimerInit( &LoraTxTestTimer, OnTxTimerEvent );
    TimerSetValue(&LoraTxTestTimer, LORA_TX_DELAY);
    OnTxTimerEvent();
    
    while(1) {
        Radio.IrqProcess( );
    }
}



void OnWakeupTimerEvent( void )
{
    //stop timer
    TimerStop(&WakeupTestTimer);
}

void wakeup_irq()
{
}

int test_case_csleep(int argc, char *argv[])
{
    uint8_t sleep_mode;
    uint8_t wakeup_mode;
    wakeup_mode = strtol((const char *)argv[0], NULL, 0);
    sleep_mode = strtol((const char *)argv[1], NULL, 0);
    
    BoardInitMcu();
   
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;

    Radio.Init( &TestRadioEvents );
    
    //lora sleep
    if(sleep_mode==0)
        Radio.Sleep();
    else {
        SleepParams_t params = { 0 };
        params.Fields.WarmStart = 0;
        SX126xSetSleep( params );
    }
    
    //set wakeup source
    if(wakeup_mode==0) {
        TimerInit( &WakeupTestTimer, OnWakeupTimerEvent );
        TimerSetValue(&WakeupTestTimer, 10000);
        TimerStart(&WakeupTestTimer);
    }else{
        Gpio_t pin_wakeup;
        GpioInit( &pin_wakeup, PE_6, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
        GpioSetInterrupt( &pin_wakeup, IRQ_FALLING_EDGE, IRQ_HIGH_PRIORITY, wakeup_irq );
    }

    printf("\r\ndeep sleep, waiting for %s...\r\n", wakeup_mode==0?"timer intr":"gpio intr");
    
    BoardDeInitMcu( );
    PWR_UltraLowPowerCmd(ENABLE); 
    halt();

    BoardInitMcu();
    printf("wakeup...\r\n");
    
    return 0;
}


int test_case_cstdby(int argc, char *argv[])
{
    Gpio_t pin_wakeup;
    uint8_t stdby_mode;
    stdby_mode = strtol((const char *)argv[0], NULL, 0);
    
    BoardInitMcu();
    TestRadioEvents.TxDone = OnTxDone;
    TestRadioEvents.RxDone = OnRxDone;
    TestRadioEvents.TxTimeout = OnTxTimeout;
    TestRadioEvents.RxTimeout = OnRxTimeout;
    TestRadioEvents.RxError = OnRxError;

    Radio.Init( &TestRadioEvents );
    
    //lora standby
    if(stdby_mode==0)
        SX126xSetStandby(STDBY_RC);
    else
        SX126xSetStandby(STDBY_XOSC);
    
    //set wakeup source  
    GpioInit( &pin_wakeup, PE_6, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );
    GpioSetInterrupt( &pin_wakeup, IRQ_FALLING_EDGE, IRQ_HIGH_PRIORITY, wakeup_irq );
    
    printf("\r\nlora standby...\r\n");
    
    BoardDeInitMcu( );
    PWR_UltraLowPowerCmd(ENABLE);  
    halt();
    
    BoardInitMcu();
    printf("wakeup...\r\n");

    return 0;
}

void mcu_lpm_active_halt(void)
{
    BoardInitMcu();
    
    Radio.Init( &TestRadioEvents );
    Radio.Sleep();
    
    printf("\r\nmcu active halt...\r\n");
    
    BoardDeInitMcu( );
    PWR_UltraLowPowerCmd(ENABLE); 
    halt();
    
    while(1);
}

void mcu_lpm_halt(void)
{
    BoardInitMcu();
    
    Radio.Init( &TestRadioEvents );
    Radio.Sleep();
    
    printf("\r\nmcu halt...\r\n");
    
    BoardDeInitMcu( );
    CLK_PeripheralClockConfig(CLK_Peripheral_RTC, DISABLE);
    CLK_RTCClockConfig(CLK_RTCCLKSource_Off, CLK_RTCCLKDiv_1);
    CLK_LSEConfig(CLK_LSE_OFF);
    while (CLK_GetFlagStatus(CLK_FLAG_LSERDY) == RESET);

    PWR_UltraLowPowerCmd(ENABLE); 
    halt();
    
    while(1);
}

void mcu_lpm_wait(void)
{
    BoardInitMcu();
    
    Radio.Init( &TestRadioEvents );
    Radio.Sleep();
    
    printf("\r\nmcu wait...\r\n");
    
    BoardDeInitMcu();
    wfe();
    
    while(1);
}

#ifdef _COSMIC_
#pragma section (CODE_IN_RAM)
void LPWM_Ram(int lpwm)
#else
__ramfunc void LPWM_Ram(int lpwm)
#endif
{   
    /* To reduce consumption to minimal 
    Swith off the Flash */
    FLASH->CR1 = 0x08;
    while(((CLK->REGCSR)&0x80)==0x80);
    
    /* Swith off the Regulator*/
    CLK->REGCSR = 0x02;
    while(((CLK->REGCSR)&0x01)==0x01);
    
    if (lpwm) {
        wfe();
    } else {
        while(1);  
    }

    //Switch on the regulator
    CLK->REGCSR = 0x00;
    while(((CLK->REGCSR)&0x1) != 0x1);		
}
/* End Section LPRUN */
#ifdef _COSMIC_
#pragma section ()
#endif

void LPWM_init(int lpwm)
{
    /*Switch the clock to LSE and disable HSI*/
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_LSE);	
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);
    CLK_HSICmd(DISABLE);
    
    sim();

#ifdef _COSMIC_
    if (!(_fctcpy('C')))
        while(1);
#endif

    LPWM_Ram(lpwm); // Call in RAM
    
    /*Switch the clock to HSI*/
    CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_8);
    CLK_HSICmd(ENABLE);
    while (((CLK->ICKCR)& 0x02)!=0x02);
    
    CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI);
    CLK_SYSCLKSourceSwitchCmd(ENABLE);
    while (((CLK->SWCR)& 0x01)==0x01);  
}

void mcu_lpm_run(void)
{
    BoardInitMcu();
    
    Radio.Init( &TestRadioEvents );
    Radio.Sleep();  
    
    printf("\r\nmcu low power run...\r\n");
    
    BoardDeInitMcu();
    LPWM_init(0);
}

void mcu_lpm_lpwait(void)
{
    BoardInitMcu();
    
    Radio.Init( &TestRadioEvents );
    Radio.Sleep();  
    
    printf("\r\nmcu low power wait...\r\n");
    
    BoardDeInitMcu();
    LPWM_init(1);
}

int test_case_cmculpm(int argc, char *argv[])
{
    uint8_t lpm;
    if (argc > 1){
        return 0;
    }
    
    lpm = strtol((const char *)argv[0], NULL, 0);
    switch(lpm) {
        case 0: mcu_lpm_active_halt();break;
        case 1: mcu_lpm_halt();break;
        case 2: mcu_lpm_run();break;
        case 3: mcu_lpm_lpwait();break;
        case 4: mcu_lpm_wait();break;
        default: break;
    }

    return 0;
}
