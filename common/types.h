#ifndef __TYPES_H
#define __TYPES_H
#include <iostream>
#include <stdlib.h>

#ifdef __linux

#else	// windows
void perror(const char* str) {
	printf("%s",str);
}

#endif



namespace CMD {	// ����ָ���sidͳһΪ0
	enum {
		ERR_SUCCESS = 0,
		ERR_ERROR,
		
		CMD_LOGIN,
		CMD_END_SESSION,
		CMD_MAX_COUNT
	};

	typedef struct {
		int _type;	// �����ö��
		int _sid;
	}dis_connect;

	char* MAKE_CMD_END_SESSION(int sid) {
		dis_connect* data = (dis_connect*)malloc(sizeof(dis_connect));
		data->_type = CMD_END_SESSION;
		data->_sid = sid;
		return (char*)data;
	}
};


#endif