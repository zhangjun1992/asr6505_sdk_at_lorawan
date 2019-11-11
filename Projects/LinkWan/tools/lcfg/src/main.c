#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "Commissioning.h"
#include "lwan_config.h"
#include "parson.h"

#define CLASS_A 0
#define CLASS_B 1
#define CLASS_C 2

#define DR_3   3

#define STM8L_UINT32(u32) (((u32<<24)&0xFF000000) | ((u32<<8)&0x00FF0000) | ((u32>>8)&0x0000FF00) | ((u32>>24)&0x000000FF))
#define STM8L_UINT16(u16) (((u16<<8)&0xFF00) | ((u16>>8)&0x00FF))

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

static uint16_t crc16(uint8_t *buffer, uint8_t length )
{
    const uint16_t polynom = 0x1021;
    uint16_t crc = 0x0000;

    for (uint8_t i = 0; i < length; ++i) {
        crc ^= ( uint16_t ) buffer[i] << 8;
        for (uint8_t j = 0; j < 8; ++j) {
            crc = (crc & 0x8000) ? (crc << 1) ^ polynom : (crc << 1);
        }
    }

    return crc;
}

static void save_eeprom_bin(LWanDevKeys_t *keys, LWanDevConfig_t *dev_config, LWanMacConfig_t *mac_config, uint8_t *eeprom_bin)
{
    uint32_t i=0;
    uint8_t *q = NULL;
    uint8_t *p = eeprom_bin;
    
    p += 16;
    
    q = (uint8_t *)keys;
    for(i=0; i<sizeof(LWanDevKeys_t); i++)
        *p++ = *q++;
    
    q = (uint8_t *)dev_config;
    for(i=0; i<sizeof(LWanDevConfig_t); i++)
        *p++ = *q++;
    
    q = (uint8_t *)mac_config;
    for(i=0; i<sizeof(LWanMacConfig_t); i++)
        *p++ = *q++;
}

static void output_intel_hex(uint8_t *eeprom_bin, char *file)
{
    const char *hex_end = ":00000001FF";
    uint16_t addr_start = 0x1000;
    uint8_t bytes_per_line = 32;
    uint8_t lines_num = 2048/bytes_per_line;
    char line[77];
    uint8_t line_bytes[37];
    uint16_t check_sum = 0;
    FILE *fp = NULL;
    
    fp = fopen(file, "w");
    if(!fp) {
        printf("failed to open hex file\r\n");
        return;
    }
    
    uint8_t *p = eeprom_bin;
    for(int i=0; i<lines_num; i++) {
        //line bytes
        uint16_t addr = addr_start + i*bytes_per_line;
        line_bytes[0] = bytes_per_line;
        line_bytes[1] = (addr >> 8) & 0xFF;
        line_bytes[2] = addr & 0xFF;
        line_bytes[3] = 0x00;
        
        check_sum = (uint16_t)line_bytes[0] + line_bytes[1] + line_bytes[2] + line_bytes[3];
        for(int j=0; j<bytes_per_line; j++) {
            line_bytes[4+j] = *p++;
            check_sum += line_bytes[4+j];
        }
        line_bytes[4+bytes_per_line] = 0x100 - (check_sum % 256);
        
        //line
        line[0] = ':';
        for(int j=0; j<37; j++) {
            sprintf((char *)line+1+j*2, "%02X", line_bytes[j]);
        }
        sprintf((char *)line+1+37*2, "\r\n");
        
        fwrite(line, 77, 1, fp);
    }
    
    fwrite(hex_end, strlen(hex_end), 1, fp);
    
    
    fclose(fp);
    
}

int main()
{
    LWanDevKeys_t keys = LWAN_DEV_KEYS_DEFAULT;
    LWanDevConfig_t dev_config = LWAN_DEV_CONFIG_DEFAULT;
    LWanMacConfig_t mac_config = LWAN_MAC_CONFIG_DEFAULT;
    const char *data = NULL;
    int value = -1;
    JSON_Value *root_value;
    uint8_t eeprom_bin[2048];
    uint8_t buf[256];
    int len = 0;
    
    root_value = json_parse_file_with_comments("config.json");
    if(!root_value)
        printf("parse config file failed\r\n");
    
    //LWanDevKeys
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.OTAKeys.DevEui");
    if(data)
        hex2bin(data, keys.ota.deveui, 8);
        
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.OTAKeys.AppEui");
    if(data)
        hex2bin(data, keys.ota.appeui, 8);
        
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.OTAKeys.AppKey");
    if(data)
        hex2bin(data, keys.ota.appkey, 16);
        
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.ABPKeys.DevAddr");
    if(data) {
        len = hex2bin((const char *)data, buf, 4);
        if(len == 4)
            keys.abp.devaddr = buf[0] << 24 | buf[1] << 16 | buf[2] <<8 | buf[3];
    }
    
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.ABPKeys.NwkSKey");
    if(data)
        hex2bin(data, keys.abp.nwkskey, 16);
        
    data = json_object_dotget_string(json_object(root_value), "LWanDevKeys.ABPKeys.AppSKey");
    if(data)
        hex2bin(data, keys.abp.appskey, 16);
    
    //LWanDevConfig
    data = json_object_dotget_string(json_object(root_value), "LWanDevConfig.JoinMode");
    if(data) {
        if(!strcmp(data, "OTAA"))
            dev_config.modes.join_mode = JOIN_MODE_OTAA;
        else
            dev_config.modes.join_mode = JOIN_MODE_ABP;
    }
    
    data = json_object_dotget_string(json_object(root_value), "LWanDevConfig.ULDLMode");
    if(data) {
        if(!strcmp(data, "INTER"))
            dev_config.modes.uldl_mode = ULDL_MODE_INTER;
        else
            dev_config.modes.uldl_mode = ULDL_MODE_INTRA;
    }
    
    data = json_object_dotget_string(json_object(root_value), "LWanDevConfig.ClassMode");
    if(data) {
        if(!strcmp(data, "CLASS_A")) {
            dev_config.modes.class_mode = CLASS_A;
        }else
            dev_config.modes.class_mode = CLASS_C;
    }
    
    data = json_object_dotget_string(json_object(root_value), "LWanDevConfig.FreqbandMask");
    if(data) {
        len = hex2bin((const char *)data, buf, 2);
        if(len == 2)
            dev_config.freqband_mask = buf[1] | ((uint16_t)buf[0] << 8);
    }
    
    //LWanMacConfig
    value = json_object_dotget_boolean(json_object(root_value), "LWanMacConfig.ConfirmedMsg");
    if(value != -1)
        mac_config.modes.confirmed_msg = value;
        
    data = json_object_dotget_string(json_object(root_value), "LWanMacConfig.Port");
    if(data) {
        mac_config.port = atoi(data);
    }    
    
    value = json_object_dotget_boolean(json_object(root_value), "LWanMacConfig.ReportMode");
    if(value != -1)
        mac_config.modes.report_mode = value;
        
    data = json_object_dotget_string(json_object(root_value), "LWanMacConfig.ReportInterval");
    if(data) {
        mac_config.report_interval = atoi(data);
    }
    
    json_value_free(root_value);
    
	//change for stm8l
	keys.magic = STM8L_UINT32(keys.magic);
	keys.abp.devaddr = STM8L_UINT32(keys.abp.devaddr);
	dev_config.magic = STM8L_UINT32(dev_config.magic);
	dev_config.freqband_mask = STM8L_UINT16(dev_config.freqband_mask);
	mac_config.magic = STM8L_UINT32(mac_config.magic);
	mac_config.rx_params.rx2_freq = STM8L_UINT32(mac_config.rx_params.rx2_freq);
	mac_config.report_interval = STM8L_UINT32(mac_config.report_interval);
	
    //update crc
    keys.checksum = crc16((uint8_t *)&keys, sizeof(LWanDevKeys_t) - 2);
    dev_config.crc = crc16((uint8_t *)&dev_config, sizeof(LWanDevConfig_t) - 2);
    mac_config.crc = crc16((uint8_t *)&mac_config, sizeof(LWanMacConfig_t)  - 2);
    keys.checksum = STM8L_UINT16(keys.checksum);
	dev_config.crc = STM8L_UINT16(dev_config.crc);
	mac_config.crc = STM8L_UINT16(mac_config.crc);
    
    //save to a buffer
    memset(eeprom_bin, 0, 2048);
    save_eeprom_bin(&keys, &dev_config, &mac_config, eeprom_bin);
    
    //hex file output
    output_intel_hex(eeprom_bin, "eeprom.hex");
    
	return 0;
}
