/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <stdint.h>
#include "linkwan.h"
#include "board.h"

LOG_LEVEL g_log_level = LL_DEBUG;

int main( void )
{
    BoardInitMcu();
    
    lora_init(NULL);  
    lora_fsm( );
    return 0;
}
