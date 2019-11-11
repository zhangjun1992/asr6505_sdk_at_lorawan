/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
typedef enum LOG_LEVEL_E {
    LL_NONE,
    LL_ERR,
    LL_WARN,
    LL_DEBUG,
    LL_VDEBUG,
    LL_ALL
} LOG_LEVEL;

#ifdef CONFIG_DEBUG_LINKWAN
#ifdef __CSMC__
    #define ERR_PRINTF printf
    #define WARN_PRINTF printf
    #define DBG_PRINTF printf
    #define VDBG_PRINTF printf
    #define PRINTF_RAW printf
    #define PRINTF_AT printf
    #define ERR_PRINTF printf
#else    
    extern LOG_LEVEL g_log_level;
    #define ERR_PRINTF(format, ...)    do { \
        if(g_log_level>=LL_ERR) { \
            TimerTime_t ts = TimerGetCurrentTime(); \
            printf("[%lu]", ts);  \
            printf(format, ##__VA_ARGS__); \
        } \
    }while(0)
    
    #define WARN_PRINTF(format, ...)    do { \
        if(g_log_level>=LL_WARN) { \
            TimerTime_t ts = TimerGetCurrentTime(); \
            printf("[%lu]", ts);  \
            printf(format, ##__VA_ARGS__); \
        } \
    }while(0)
    
    #define DBG_PRINTF(format, ...)    do { \
        if(g_log_level>=LL_DEBUG) { \
            TimerTime_t ts = TimerGetCurrentTime(); \
            printf("[%lu]", ts);  \
            printf(format, ##__VA_ARGS__); \
        } \
    }while(0)
    
    #define VDBG_PRINTF(format, ...)    do { \
        if(g_log_level>=LL_VDEBUG) { \
            TimerTime_t ts = TimerGetCurrentTime(); \
            printf("[%lu]", ts);  \
            printf(format, ##__VA_ARGS__); \
        } \
    }while(0)
    
    #define PRINTF_RAW(...) do { \
        if(g_log_level>=LL_DEBUG) printf(__VA_ARGS__); \
    }while(0)
    
    #define PRINTF_AT DBG_PRINTF
    
    #define DBG_PRINTF_CRITICAL(p)
#endif
#else
    #define ERR_PRINTF(format, ...)    do {}while(0)
    #define WARN_PRINTF(format, ...)    do {}while(0)
    #define DBG_PRINTF(format, ...)    do {}while(0)
    #define VDBG_PRINTF(format, ...)    do {}while(0)
    #define PRINTF_RAW(...) do {}while(0)
    #define PRINTF_AT(...) do {}while(0)
    #define DBG_PRINTF_CRITICAL(p)
#endif


/* Exported functions ------------------------------------------------------- */

/**
 * @brief  Initializes the debug
 * @param  None
 * @retval None
 */
void DBG_Init(void);
int DBG_LogLevelGet();
void DBG_LogLevelSet(int level);

#ifdef __cplusplus
}
#endif

#endif /* __DEBUG_H__*/

