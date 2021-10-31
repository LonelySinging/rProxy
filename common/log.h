#ifndef _LOG_H
#define _LOG_H
#ifdef __linux
;
#else
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

static char* GetLogTime() {
    static char buf[128];
    memset(buf, 0, sizeof(buf));
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", timeinfo);
    return buf;
}

#define __DTIME__ (GetLogTime())

//#define LOG_I(format, ...)\
//do{\
//    fprintf(stdout,"[%s I] %s:%d: "#format"\b \n", __DTIME__, __FILENAME__, __LINE__, ##__VA_ARGS__); \
//}while(0)

#define LOG_I(...)  do { fprintf(stdout,"[%s I]: ", __DTIME__); fprintf(stdout, __VA_ARGS__); fprintf(stdout, "\n"); } while(0)
#define LOG_D(...)  do { /*fprintf(stderr,"[%s D] %s:%d: ", __DTIME__, __FILENAME__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");*/ } while(0)
#define LOG_W(...)  do { fprintf(stderr,"[%s W] %s:%d: ", __DTIME__, __FILENAME__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)
#define LOG_E(...)  do { fprintf(stderr,"[%s E] %s:%d: ", __DTIME__, __FILENAME__, __LINE__); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } while(0)

//#define LOG_D(format, ...)\
////do{fprintf(stderr,"[%s D] %s:%d: "#format "\b \n", __DTIME__, __FILENAME__, __LINE__, ##__VA_ARGS__);}while(0)
//
//#define LOG_W(format, ...)\
//do{\
//    fprintf(stderr,"[%s W] %s:%d: "#format "\b \n", __DTIME__, __FILENAME__, __LINE__, ##__VA_ARGS__); \
//}while(0)
//
//#define LOG_E(format, ...)\
//do{\
//    fprintf(stderr,"[%s E] %s:%d: err_code: %d "#format "\b \n", __DTIME__, __FILENAME__, __LINE__, GetLastError(), ##__VA_ARGS__); \
//}while(0)

#endif
#endif // !_LOG_H

// https://blog.csdn.net/lwwl12/article/details/115351663