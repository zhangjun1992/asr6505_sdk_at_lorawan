/**
  ******************************************************************************
  * @file    USART/USART_Printf/main.c
  * @author  MCD Application Team
  * @version V1.5.2
  * @date    30-September-2014
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <string.h>
#include "stm8l15x.h"
#include "stm8l15x_gpio.h"
#include "board.h"
#include "gpio-board.h"
#include "stm8l15x_rtc.h"
#include "stm8l15x_tim1.h"
#include "stm8l15x_iwdg.h"
#include "stm8l15x_wwdg.h"
#include "stm8l15x_aes.h"
#include "radio.h"    
#include "test_cases.h"
#include "timer.h"

/** @addtogroup STM8L15x_StdPeriph_Examples
  * @{
  */

/**
  * @addtogroup USART_Printf
  * @{
  */


/* Private functions ---------------------------------------------------------*/

//HW 
#define AT_PROMPT "ASR6505:~#"
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
static TestCaseSt gCases[] = {
    { "AT+CTXCW=", &test_case_ctxcw },
    { "AT+CTX=", &test_case_ctx },
    { "AT+CRXS=", &test_case_crxs },
    { "AT+CRX=", &test_case_crx },
    { "AT+CSLEEP=", &test_case_csleep },
    { "AT+CSTDBY=", &test_case_cstdby },
    { "AT+CMCULPM=", &test_case_cmculpm }
};

#ifdef _COSMIC_ 
static char *olds = NULL;
char * strtok (char *s, const char *delim)
{
    char *token;
    if (s == NULL) s = olds;

    s += strspn (s, delim);
    if (*s == '\0') {
        olds = s;
        return NULL;
    }

    token = s;
    s = strpbrk (token, delim);  
    if (s == NULL)
        olds = strchr(token, '\0');
    else {      
        *s = '\0';        
        olds = s + 1;
    }
    return token;
}
#endif

int mini_cmd_line(void)
{
    int ret = -1;
    int i = 0;
    char *ptr = NULL;
    char ch;
    char cmd_str[64];
    int cmd_index = 0;
    int case_num = sizeof(gCases)/sizeof(TestCaseSt);
    int argc = 0;
	char *argv[16];
    char *str = NULL;
    char resetFlag = 1;
    
    while (1) {
        cmd_index = 0;
        argc = 0;
        if (resetFlag == 1)
        {
            memset(cmd_str, 0, sizeof(cmd_str));
            resetFlag = 0;
        }
        ch = '\0';
        printf("\r\n%s", AT_PROMPT);
        while (ch == '\0' || (ch != '\r' && ch != '\n')) {
            resetFlag = 1;
            ch = getchar();
            if (ch != '\0' && ch != '\r' && ch != '\n')
                cmd_str[cmd_index++] = ch;
        }
        if (cmd_index == 0)
            continue;
        cmd_str[cmd_index] = '\0';
             
        for (i=0; i<case_num; i++) {
            int cmd_len = strlen(gCases[i].name);
            if (!strncmp((const char *)cmd_str, gCases[i].name, cmd_len)) {
                ptr = (char *)cmd_str + cmd_len;
                break;
            }
        }
        
        if (i >= case_num || !gCases[i].fn)
            goto end;
        
        str = strtok((char *)ptr, ",");
        while(str) {
            argv[argc++] = str;
            str = strtok((char *)NULL, ",");
        }
		ret = gCases[i].fn(argc, argv);
        
end:         
        if(ret==-1) 
            printf("\r\n+CME ERROR:1\r\n");
    }
}

void main(void)
{
    BoardInitMcu();
	BoardInitPeriph();

    printf("\r\n\/****************************\r\n"
            "*****************************\r\n"
            "*       ASR6505 Test        *\r\n"
            "*****************************\r\n"
            "****************************/\r\n");
	mini_cmd_line();
    while (1)
    {
    }
}

#ifdef  USE_FULL_ASSERT
  /**
    * @brief  Reports the name of the source file and the source line number
    *   where the assert_param error has occurred.
    * @param  file: pointer to the source file name
    * @param  line: assert_param error line source number
    * @retval None
    */
  void assert_failed(uint8_t* file, uint32_t line)
  {
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
  }
#endif
/**
  * @}
  */

/**
  * @}
  */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
