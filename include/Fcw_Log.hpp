#pragma once 
#include <time.h>
#include <cstring>
#include <string>
#define INF 0
#define DBG 1
#define ERR 2
#define DEFAULT_LOG_LEVEL DBG
#define LOG(level, format, ...){\
        if (level >= DEFAULT_LOG_LEVEL){\
            time_t t = time(NULL);\
            struct tm *m = localtime(&t);\
            char ts[32] = {0};\
            strftime(ts, 31, "%H:%M:%S", m);\
            fprintf(stdout, "[%p %s %s:%d] " format "\n", (void*)pthread_self(),ts, __FILE__, __LINE__, ##__VA_ARGS__);\
        }\
}
#define INF_LOG(format, ...) LOG(INF, format, ##__VA_ARGS__);
#define DBG_LOG(format, ...) LOG(DBG, format, ##__VA_ARGS__);
#define ERR_LOG(format, ...) LOG(ERR, format, ##__VA_ARGS__);