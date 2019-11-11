/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "hw.h"
#include "LoRaMac.h"
#include "Region.h"

#include "lwan_config.h"
#include "linkwan.h"

#include "at_command.h"
#include "rtc-board.h"
#include "delay.h"
#include "board.h"

#define ARGC_LIMIT 16

#define PORT_LEN 4

#define QUERY_CMD		0x01
#define EXECUTE_CMD		0x02
#define DESC_CMD        0x03
#define SET_CMD			0x04

#define CONFIG_MANUFACTURER "ASR"
#define CONFIG_DEVICE_MODEL "6505"

extern uint8_t atcmd[ATCMD_SIZE];
extern uint16_t atcmd_index;
extern bool g_atcmd_processing;
bool g_lpm_status = false;
uint8_t g_default_key[LORA_KEY_LENGTH] = {0x41, 0x53, 0x52, 0x36, 0x35, 0x30, 0x58, 0x2D, 
                                          0x32, 0x30, 0x31, 0x38, 0x31, 0x30, 0x33, 0x30};

typedef struct {
	char *cmd;
	int (*fn)(int opt, int argc, char *argv[]);	
}at_cmd_t;

//AT functions
static int at_cjoinmode_func(int opt, int argc, char *argv[]);
static int at_cdeveui_func(int opt, int argc, char *argv[]);
static int at_cappeui_func(int opt, int argc, char *argv[]);
static int at_cappkey_func(int opt, int argc, char *argv[]);
static int at_cdevaddr_func(int opt, int argc, char *argv[]);
static int at_cappskey_func(int opt, int argc, char *argv[]);
static int at_cnwkskey_func(int opt, int argc, char *argv[]);
static int at_cfreqbandmask_func(int opt, int argc, char *argv[]);
static int at_culdlmode_func(int opt, int argc, char *argv[]);
static int at_cclass_func(int opt, int argc, char *argv[]);
static int at_cstatus_func(int opt, int argc, char *argv[]);
static int at_cjoin_func(int opt, int argc, char *argv[]);
static int at_dtrx_func(int opt, int argc, char *argv[]);
static int at_drx_func(int opt, int argc, char *argv[]);
static int at_cconfirm_func(int opt, int argc, char *argv[]);
static int at_cappport_func(int opt, int argc, char *argv[]);
static int at_cdatarate_func(int opt, int argc, char *argv[]);
static int at_cnbtrials_func(int opt, int argc, char *argv[]);
static int at_crm_func(int opt, int argc, char *argv[]);
static int at_ctxp_func(int opt, int argc, char *argv[]);
static int at_clinkcheck_func(int opt, int argc, char *argv[]);
static int at_cadr_func(int opt, int argc, char *argv[]);
static int at_crxp_func(int opt, int argc, char *argv[]);
static int at_crx1delay_func(int opt, int argc, char *argv[]);
static int at_ckeysprotect_func(int opt, int argc, char *argv[]);
static int at_clpm_func(int opt, int argc, char *argv[]);



static at_cmd_t g_at_table[] = {
    {LORA_AT_CJOINMODE, at_cjoinmode_func},
    {LORA_AT_CDEVEUI,  at_cdeveui_func},
    {LORA_AT_CAPPEUI,  at_cappeui_func},
    {LORA_AT_CAPPKEY,  at_cappkey_func},
    {LORA_AT_CDEVADDR,  at_cdevaddr_func},
    {LORA_AT_CAPPSKEY,  at_cappskey_func},
    {LORA_AT_CNWKSKEY,  at_cnwkskey_func},
    {LORA_AT_CFREQBANDMASK,  at_cfreqbandmask_func},
    {LORA_AT_CULDLMODE,  at_culdlmode_func},
    {LORA_AT_CCLASS,  at_cclass_func},
    {LORA_AT_CSTATUS,  at_cstatus_func},
    {LORA_AT_CJOIN, at_cjoin_func},
    {LORA_AT_DTRX,  at_dtrx_func},
    {LORA_AT_DRX,  at_drx_func},
    {LORA_AT_CCONFIRM,  at_cconfirm_func},
    {LORA_AT_CAPPPORT,  at_cappport_func},
    {LORA_AT_CDATARATE,  at_cdatarate_func},
    {LORA_AT_CNBTRIALS,  at_cnbtrials_func},
    {LORA_AT_CRM,  at_crm_func},
    {LORA_AT_CTXP,  at_ctxp_func},
    {LORA_AT_CLINKCHECK,  at_clinkcheck_func},
    {LORA_AT_CADR,  at_cadr_func},
    {LORA_AT_CRXP,  at_crxp_func},
    {LORA_AT_CRX1DELAY,  at_crx1delay_func},
    {LORA_AT_CLPM,  at_clpm_func}
};

#define AT_TABLE_SIZE	(sizeof(g_at_table) / sizeof(at_cmd_t))

static int hex2bin(const char *hex, uint8_t *bin, uint16_t bin_length)
{
    uint16_t hex_length = strlen(hex);
    const char *hex_end = hex + hex_length;
    uint8_t *cur = bin;
    uint8_t num_chars = hex_length & 1;
    uint8_t byte = 0;

    if ((hex_length + 1) / 2 > bin_length) {
        return -1;
    }

    while (hex < hex_end) {
        if ('A' <= *hex && *hex <= 'F') {
            byte |= 10 + (*hex - 'A');
        } else if ('a' <= *hex && *hex <= 'f') {
            byte |= 10 + (*hex - 'a');
        } else if ('0' <= *hex && *hex <= '9') {
            byte |= *hex - '0';
        } else {
            return -1;
        }
        hex++;
        num_chars++;

        if (num_chars >= 2) {
            num_chars = 0;
            *cur++ = byte;
            byte = 0;
        } else {
            byte <<= 4;
        }
    }
    return cur - bin;
}

int linkwan_serial_output(uint8_t *buffer, int len)
{
    PRINTF_AT("%s", buffer);
    linkwan_at_prompt_print();
    return 0;
}

void linkwan_at_prompt_print(void)
{
    PRINTF_RAW("\r\n%s%s:~# ", CONFIG_MANUFACTURER, CONFIG_DEVICE_MODEL);
}

void LowPower_Disable( void )
{
    if (g_lpm_status == false)
    {
       return ;
    }
    g_lpm_status = false;
    DisableLowPowerDuringTask(true);
}

void LowPower_Enable( void )
{
    if (g_lpm_status == true)
    {
       return ;
    }
    g_lpm_status = true;
    DisableLowPowerDuringTask(false);
}

static int at_cjoinmode_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    int res = LWAN_ERROR;
    uint8_t join_mode;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_JOIN_MODE, &join_mode);  
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CJOINMODE, join_mode);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"mode\"\r\nOK\r\n", LORA_AT_CJOINMODE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            join_mode = strtol((const char *)argv[0], NULL, 0);
            res = lwan_dev_config_set(DEV_CONFIG_JOIN_MODE, (void *)&join_mode);
            if (res == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cdeveui_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_DEVEUI, buf);
            sprintf((char *)atcmd, "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
										LORA_AT_CDEVEUI, (uint16_t)buf[0], (uint16_t)buf[1], (uint16_t)buf[2], (uint16_t)buf[3], \
										(uint16_t)buf[4], (uint16_t)buf[5], (uint16_t)buf[6], (uint16_t)buf[7]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=\"DevEUI:length is 16\"\r\n", LORA_AT_CDEVEUI);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], buf, LORA_EUI_LENGTH);
            if (length == LORA_EUI_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_OTA_DEVEUI, buf) == LWAN_SUCCESS) {
                    sprintf((char *)atcmd, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cappeui_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_APPEUI, buf);
            sprintf((char *)atcmd, "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CAPPEUI, (uint16_t)buf[0], (uint16_t)buf[1], (uint16_t)buf[2], (uint16_t)buf[3], (uint16_t)buf[4], (uint16_t)buf[5], (uint16_t)buf[6], (uint16_t)buf[7]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=\"AppEUI:length is 16\"\r\n", LORA_AT_CAPPEUI);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_EUI_LENGTH);
            if (length == LORA_EUI_LENGTH && lwan_dev_keys_set(DEV_KEYS_OTA_APPEUI, buf) == LWAN_SUCCESS) {
                sprintf((char *)atcmd, "\r\nOK\r\n");
                ret = LWAN_SUCCESS;
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cappkey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_OTA_APPKEY, buf);   
            sprintf((char *)atcmd, 
                     "\r\n%s:%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\r\nOK\r\n", \
                     LORA_AT_CAPPKEY, (uint16_t)buf[0], (uint16_t)buf[1], (uint16_t)buf[2], (uint16_t)buf[3], (uint16_t)buf[4], (uint16_t)buf[5], (uint16_t)buf[6], (uint16_t)buf[7],
                                      (uint16_t)buf[8], (uint16_t)buf[9], (uint16_t)buf[10], (uint16_t)buf[11], (uint16_t)buf[12], (uint16_t)buf[13], (uint16_t)buf[14], (uint16_t)buf[15]);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=\"AppKey:length is 32\"\r\n", LORA_AT_CAPPKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH && lwan_dev_keys_set(DEV_KEYS_OTA_APPKEY, buf) == LWAN_SUCCESS) {
                sprintf((char *)atcmd, "\r\nOK\r\n");
                ret = LWAN_SUCCESS;
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cdevaddr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
    uint32_t devaddr;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_ABP_DEVADDR, &devaddr);
            length = sprintf((char *)atcmd, "\r\n%s:%08X\r\nOK\r\n", LORA_AT_CDEVADDR, (unsigned int)devaddr);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=<DevAddr:length is 8, Device address of ABP mode>\r\n", LORA_AT_CDEVADDR);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], buf, 4);
            if (length == 4) {
                uint32_t devaddr = buf[0] << 24 | buf[1] << 16 | buf[2] <<8 | buf[3];
                if(lwan_dev_keys_set(DEV_KEYS_ABP_DEVADDR, &devaddr) == LWAN_SUCCESS) {
                    sprintf((char *)atcmd, "\r\nOK\r\n");
                    ret = LWAN_SUCCESS;
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cappskey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
    int i;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_ABP_APPSKEY, buf); 
            length = sprintf((char *)atcmd, "\r\n%s:", LORA_AT_CAPPSKEY);
            for (i = 0; i < LORA_KEY_LENGTH; i++) {
                length += sprintf((char *)(atcmd + length), "%02X", (uint16_t)buf[i]);
            }
            sprintf((char *)(atcmd + length), "\r\nOK\r\n");
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=<AppSKey: length is 32>\r\n", LORA_AT_CAPPSKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_ABP_APPSKEY, buf) == LWAN_SUCCESS) {
                    sprintf((char *)atcmd, "\r\nOK\r\n");
                    ret = true;
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cnwkskey_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
    int i;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_ABP_NWKSKEY, buf); 
            length = sprintf((char *)atcmd, "\r\n%s:", LORA_AT_CNWKSKEY);
            for (i = 0; i < LORA_KEY_LENGTH; i++) {
                length += sprintf((char *)(atcmd + length), "%02X", (uint16_t)buf[i]);
            }
            sprintf((char *)(atcmd + length), "\r\nOK\r\n");
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=<NwkSKey:length is 32>\r\n", LORA_AT_CNWKSKEY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                if(lwan_dev_keys_set(DEV_KEYS_ABP_NWKSKEY, buf) == LWAN_SUCCESS) {
                    sprintf((char *)atcmd, "\r\nOK\r\n");
                    ret = true;
                }
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_cfreqbandmask_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint16_t freqband_mask;
    uint8_t mask[2];
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_FREQBAND_MASK, &freqband_mask);  
            sprintf((char *)atcmd, "\r\n%s:%04X\r\nOK\r\n", LORA_AT_CFREQBANDMASK, freqband_mask);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=\"mask\"\r\nOK\r\n", LORA_AT_CFREQBANDMASK);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], (uint8_t *)mask, 2);
            if (length == 2) {
                freqband_mask = mask[1] | ((uint16_t)mask[0] << 8);
                if (lwan_dev_config_set(DEV_CONFIG_FREQBAND_MASK, (void *)&freqband_mask) == LWAN_SUCCESS) {
                    ret = LWAN_SUCCESS;
                    sprintf((char *)atcmd, "\r\nOK\r\n");
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_culdlmode_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t mode;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_ULDL_MODE, &mode);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CULDLMODE, (uint16_t)mode);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=\"mode\"\r\nOK\r\n", LORA_AT_CULDLMODE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            mode = strtol((const char *)argv[0], NULL, 0);
            if (lwan_dev_config_set(DEV_CONFIG_ULDL_MODE, (void *)&mode) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cclass_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t class;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_config_get(DEV_CONFIG_CLASS, &class);  
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CCLASS, (uint16_t)class);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, 
                     "\r\n%s:\"class\",\"branch\",\"para1\",\"para2\",\"para3\",\"para4\"\r\nOK\r\n",
                     LORA_AT_CCLASS);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            class = strtol((const char *)argv[0], NULL, 0);
            if (lwan_dev_config_set(DEV_CONFIG_CLASS, (void *)&class) == LWAN_SUCCESS) {
								ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }

            break;
        }
        default: break;
    }

    return ret;
}

static int at_cstatus_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    int status;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            status = lwan_dev_status_get();
            sprintf((char *)atcmd, "\r\n%s:%02d\r\nOK\r\n", LORA_AT_CSTATUS, status);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"status\"\r\nOK\r\n", LORA_AT_CSTATUS);
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_dtrx_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
		uint8_t confirm, Nbtrials;
		uint16_t len;
    int bin_len = 0;
    uint8_t payload[LORAWAN_APP_DATA_BUFF_SIZE];
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:[confirm],[nbtrials],[length],<payload>\r\nOK\r\n", LORA_AT_DTRX);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;

            confirm = strtol((const char *)argv[0], NULL, 0);
            Nbtrials = strtol((const char *)argv[1], NULL, 0);
            len = strtol((const char *)argv[2], NULL, 0);
            bin_len = hex2bin((const char *)argv[3], payload, len);
            
            if(bin_len>=0) {
                ret = LWAN_SUCCESS;
                if (lwan_data_send(confirm, Nbtrials, payload, bin_len) == LWAN_SUCCESS) {
                    sprintf((char *)atcmd, "\r\nOK+SEND:%02X\r\n", (uint16_t)bin_len);
                }else{
                    sprintf((char *)atcmd, "\r\nERR+SEND:00\r\n");
                }
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_drx_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    int16_t len;
    uint8_t port;
    uint8_t *buf;
    uint8_t size;
    int i;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:<Length>,<Payload>\r\nOK\r\n", LORA_AT_DRX);
            break;
        }
        case QUERY_CMD: {
                    
            if(lwan_data_recv(&port, &buf, &size) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                len = sprintf((char *)atcmd, "\r\n%s:%d", LORA_AT_DRX, size);
                if (size > 0) {
                    len += sprintf((char *)(atcmd + len), ",");
                    for(i=0; i<size; i++) {
                        len += sprintf((char *)(atcmd + len), "%02X", (uint16_t)buf[i]);
                    }
                }
                sprintf((char *)(atcmd + len), "\r\n%s\r\n", "OK");
            }
            break;
        }
        default: break;
    }

    return ret;

}


static int at_cconfirm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t cfm;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_CONFIRM_MSG, &cfm);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CCONFIRM, cfm);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CCONFIRM);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            cfm = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_CONFIRM_MSG, (void *)&cfm) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cappport_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t port;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_APP_PORT, &port);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CAPPPORT, port);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CAPPPORT);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            port = (uint8_t)strtol((const char *)argv[0], NULL, 0);

            if (lwan_mac_config_set(MAC_CONFIG_APP_PORT, (void *)&port) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_cdatarate_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t datarate;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_DATARATE, &datarate);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CDATARATE, datarate);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CDATARATE);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            datarate = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_DATARATE, (void *)&datarate) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cnbtrials_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t m_type, value;
    int res = LWAN_SUCCESS;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_CONFIRM_MSG, &m_type);
            if(m_type)
                lwan_mac_config_get(MAC_CONFIG_CONF_NBTRIALS, &value);
            else
                lwan_mac_config_get(MAC_CONFIG_UNCONF_NBTRIALS, &value);
            sprintf((char *)atcmd, "\r\n%s:%d,%d\r\nOK\r\n", LORA_AT_CNBTRIALS, m_type, value);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"MTypes\",\"value\"\r\nOK\r\n", LORA_AT_CNBTRIALS);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;
            
            m_type = strtol((const char *)argv[0], NULL, 0);
            value = strtol((const char *)argv[1], NULL, 0);
            if(m_type)
                res = lwan_mac_config_set(MAC_CONFIG_CONF_NBTRIALS, (void *)&value);   
            else
                res = lwan_mac_config_set(MAC_CONFIG_UNCONF_NBTRIALS, (void *)&value);
            if (res == LWAN_SUCCESS) { 
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}


static int at_crm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t reportMode;
    uint32_t reportInterval;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_REPORT_MODE, &reportMode);
            lwan_mac_config_get(MAC_CONFIG_REPORT_INTERVAL, &reportInterval);
            sprintf((char *)atcmd, "\r\n%s:%d,%u\r\nOK\r\n", LORA_AT_CRM, reportMode, (unsigned int)reportInterval);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"reportMode\",\"reportInterval\"\r\nOK\r\n", LORA_AT_CRM);
            break;
        }
        case SET_CMD: {
            if(argc < 2) break;
            
            reportMode = strtol((const char *)argv[0], NULL, 0);
            reportInterval = strtol((const char *)argv[1], NULL, 0);
            lwan_mac_config_set(MAC_CONFIG_REPORT_MODE, (void *)&reportMode);
            if (lwan_mac_config_set(MAC_CONFIG_REPORT_INTERVAL, (void *)&reportInterval) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }

            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_ctxp_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t tx_power;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_TX_POWER, &tx_power);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CTXP, tx_power);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CTXP);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            tx_power = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_TX_POWER, (void *)&tx_power) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_clinkcheck_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t checkValue;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CLINKCHECK);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            checkValue = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_CHECK_MODE, (void *)&checkValue) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                if(checkValue==1)
                    lwan_mac_req_send(MAC_REQ_LINKCHECK, 0);
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_cadr_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t adr;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_ADR_ENABLE, &adr);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CADR, adr);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CADR);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            adr = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_ADR_ENABLE, (void *)&adr) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }

    return ret;
}

static int at_crxp_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    RxParams_t rx_params;
    uint8_t rx1_dr_offset;
    uint8_t rx2_dr;
    uint32_t rx2_freq; 
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_RX_PARAM, &rx_params);  
            sprintf((char *)atcmd, "\r\n%s:%u,%u,%u\r\nOK\r\n", LORA_AT_CRXP, rx_params.rx1_dr_offset, rx_params.rx2_dr, (unsigned int)rx_params.rx2_freq);            
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"RX1DRoffset\",\"RX2DataRate\",\"RX2Frequency\"\r\nOK\r\n", LORA_AT_CRXP);
            break;
        }
        case SET_CMD: {
            if(argc < 3) break;

            rx1_dr_offset = strtol((const char *)argv[0], NULL, 0);
            rx2_dr = strtol((const char *)argv[1], NULL, 0);
            rx2_freq = strtol((const char *)argv[2], NULL, 0);
            rx_params.rx1_dr_offset = rx1_dr_offset;
            rx_params.rx2_dr = rx2_dr;
            rx_params.rx2_freq = rx2_freq;
            if (lwan_mac_config_set(MAC_CONFIG_RX_PARAM, (void *)&rx_params) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }     
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_crx1delay_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint16_t rx1delay;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_mac_config_get(MAC_CONFIG_RX1_DELAY, &rx1delay);
            sprintf((char *)atcmd, "\r\n%s:%u\r\nOK\r\n", LORA_AT_CRX1DELAY, (unsigned int)rx1delay);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"value\"\r\nOK\r\n", LORA_AT_CRX1DELAY);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            rx1delay = strtol((const char *)argv[0], NULL, 0);
            if (lwan_mac_config_set(MAC_CONFIG_RX1_DELAY, (void *)&rx1delay) == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
 
    return ret;
}

static int at_ckeysprotect_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    uint8_t length;
    uint8_t buf[16];
    int res = LWAN_SUCCESS;
    bool protected;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = LWAN_SUCCESS;
            lwan_dev_keys_get(DEV_KEYS_PKEY, buf);
            protected = lwan_is_key_valid(buf, LORA_KEY_LENGTH);
            sprintf((char *)atcmd, "\r\n%s:%d\r\nOK\r\n", LORA_AT_CKEYSPROTECT, protected);
            break;
        }
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s=<ProtectKey:length is 32>\r\n", LORA_AT_CKEYSPROTECT);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            length = hex2bin((const char *)argv[0], buf, LORA_KEY_LENGTH);
            if (length == LORA_KEY_LENGTH) {
                res = lwan_dev_keys_set(DEV_KEYS_PKEY, buf);
            } else {
                res = lwan_dev_keys_set(DEV_KEYS_PKEY, g_default_key);
            }
            
            if (res == LWAN_SUCCESS) {
                ret = LWAN_SUCCESS;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_clpm_func(int opt, int argc, char *argv[])
{
    int ret = LWAN_ERROR;
    int8_t mode;
        
    switch(opt) {
        case DESC_CMD: {
            ret = LWAN_SUCCESS;
            sprintf((char *)atcmd, "\r\n%s:\"Mode\"\r\nOK\r\n", LORA_AT_CLPM);
            break;
        }
        case SET_CMD: {
            if(argc < 1) break;
            
            ret = LWAN_SUCCESS;
            mode = strtol((const char *)argv[0], NULL, 0);
            if (mode) {
                LowPower_Enable();
            } else {
                LowPower_Disable();
            }
            sprintf((char *)atcmd, "\r\nOK\r\n");
            break;
        }
        default: break;
    }
    
    return ret;
}

static int at_cjoin_func(int opt, int argc, char *argv[])
{
    int ret = -1;
    uint8_t bJoin, bAutoJoin;
    uint16_t joinInterval, joinRetryCnt;
    int res;
    JoinSettings_t join_settings;
        
    switch(opt) {
        case QUERY_CMD: {
            ret = 0;
            lwan_dev_config_get(DEV_CONFIG_JOIN_SETTINGS, &join_settings);
            sprintf((char *)atcmd, "\r\n%s:%d,%d,%d,%d\r\nOK\r\n", LORA_AT_CJOIN, join_settings.auto_join, 
                     join_settings.auto_join, join_settings.join_interval, join_settings.join_trials);
            break;
        }
        case DESC_CMD: {
            ret = 0;
            sprintf((char *)atcmd, "\r\n%s:<ParaTag1>,[ParaTag2],[ParaTag3],[ParaTag4]\r\nOK\r\n", LORA_AT_CJOIN);
            break;
        }
        case SET_CMD: {
            if(argc < 4) break;
            
            bJoin = strtol((const char *)argv[0], NULL, 0);
            bAutoJoin = strtol((const char *)argv[1], NULL, 0);
            joinInterval = strtol((const char *)argv[2], NULL, 0);
            joinRetryCnt = strtol((const char *)argv[3], NULL, 0);
            
            res = lwan_join(bJoin, bAutoJoin, joinInterval, joinRetryCnt);
            if (res == LWAN_SUCCESS) {
                ret = 0;
                sprintf((char *)atcmd, "\r\nOK\r\n");
            }    
            break;
        }
        default: break;
    }
    
    return ret;
}

void linkwan_at_process(void)
{
    char *ptr = NULL;
	  int argc = 0;
	  int index = 0;
	  char *argv[ARGC_LIMIT];
    int ret = LWAN_ERROR;
    uint8_t *rxcmd = atcmd + 2;
	  char *str = NULL;
		char ch;
    int i;
    int cmd_len;
		
	  if (atcmd_index == 0)
				return;
    ch = atcmd[atcmd_index - 1];
    if (atcmd_index <=2 && ((ch == '\r') || (ch == '\n'))) {
        linkwan_at_prompt_print();
        atcmd_index = 0;
        memset(atcmd, 0xff, ATCMD_SIZE);
        return;
    }
    if (ch == '\0' || (ch != '\r' && ch != '\n')) {
				return;
    }
    g_atcmd_processing = true;
    for (i = 1; i <= 2; i++)
    {
			  if ((atcmd[atcmd_index - i] == '\r') || (atcmd[atcmd_index - i] == '\n'))
				{
					  atcmd[atcmd_index - i] = '\0';
				}
	  }
    
    if(atcmd[0] != 'A' || atcmd[1] != 'T')
        goto at_end;
    for (index = 0; index < AT_TABLE_SIZE; index++) {
        cmd_len = strlen(g_at_table[index].cmd);
    	if (!strncmp((const char *)rxcmd, g_at_table[index].cmd, cmd_len)) {
    		ptr = (char *)rxcmd + cmd_len;
    		break;
    	}
    }
	if (index >= AT_TABLE_SIZE || !g_at_table[index].fn)
        goto at_end;

    if ((ptr[0] == '?') && (ptr[1] == '\0')) {
		ret = g_at_table[index].fn(QUERY_CMD, argc, argv);
	} else if (ptr[0] == '\0') {
		ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
	}  else if (ptr[0] == ' ') {
        argv[argc++] = ptr;
		ret = g_at_table[index].fn(EXECUTE_CMD, argc, argv);
	} else if ((ptr[0] == '=') && (ptr[1] == '?') && (ptr[2] == '\0')) {
        ret = g_at_table[index].fn(DESC_CMD, argc, argv);
	} else if (ptr[0] == '=') {
		ptr += 1;
        
        str = strstr((char *)ptr, ",");
        if (str == NULL)
        {
						argv[argc++] = ptr;
			  }
				else
        {
            while(str) {
								argv[argc++] = ptr;
                str[0] = '\0';
					      ptr = str + 1;
                str = strstr((char *)ptr, ",");
            }
						argv[argc++] = ptr;
			  }
		ret = g_at_table[index].fn(SET_CMD, argc, argv);
	} else {
		ret = LWAN_ERROR;
	}

at_end:
	if (LWAN_ERROR == ret)
        sprintf((char *)atcmd, "\r\n%s%x\r\n", AT_ERROR, 1);
    
    linkwan_serial_output(atcmd, strlen((const char *)atcmd));        
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
    g_atcmd_processing = false;        
    return;
}

void linkwan_at_init(void)
{
    atcmd_index = 0;
    memset(atcmd, 0xff, ATCMD_SIZE);
}




