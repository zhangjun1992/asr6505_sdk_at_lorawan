/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <stdint.h>
#include "linkwan.h"
#include "board.h"

LOG_LEVEL g_log_level = LL_DEBUG;
#define LORAWAN_APP_PORT 100

static void LoraTxData(lora_AppData_t *AppData);
static void LoraRxData(lora_AppData_t *AppData);

static LoRaMainCallback_t LoRaMainCallbacks = {
    NULL,
    NULL,
    NULL,
    LoraTxData,
    LoraRxData
};

static void LoraTxData( lora_AppData_t *AppData)
{
    AppData->BuffSize = sprintf( (char *) AppData->Buff, "linklora asr data++");
    printf("tx: %s\r\n", AppData->Buff );
    AppData->Port = LORAWAN_APP_PORT;
}

static void LoraRxData( lora_AppData_t *AppData )
{
		int i;

    AppData->Buff[AppData->BuffSize] = '\0';
    printf( "rx: port = %d, len = %d\r\n", AppData->Port, AppData->BuffSize);
    for (i = 0; i < AppData->BuffSize; i++) {
        printf("0x%x ", AppData->Buff[i] );
    }
    printf("\r\n");
}

int main( void )
{
    BoardInitMcu();
    
    lora_init(&LoRaMainCallbacks);  
    lora_fsm( );
    return 0;
}
