#ifndef __TYPES_H
#define __TYPES_H
#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

typedef unsigned short int us16;

#ifdef __linux
	;
#else	// windows
void perror(const char* str);

#endif

namespace CMD {	// 控制指令的sid统一为0
	enum {
		ERR_SUCCESS = 0,
		ERR_ERROR,
		
		CMD_LOGIN,
		CMD_END_SESSION,
		CMD_MAX_COUNT
	};

	typedef struct {
        us16 _raw_sid = 0;
		us16 _type = CMD_END_SESSION;	// 上面的枚举
		us16 _sid;
	}cmd_dis_connect;

    typedef struct {
        enum {
            PASSWD_LEN = 64,
            DES_LEN = 128
        };
        us16 _raw_sid = 0;
        us16 _type = CMD_LOGIN;
        char _passwd[PASSWD_LEN];   // 口令
        char _describe[DES_LEN];   // 描述
    }cmd_login;

	// 构造命令数据包
	char* MAKE_cmd_dis_connect(int sid);
    char* MAKE_cmd_login(char* passwd, char* des);
};


#endif
