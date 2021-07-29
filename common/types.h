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
	}cmd_dis_connect;

	// �����������ݰ�
	char* MAKE_cmd_dis_connect(int sid) {
		cmd_dis_connect* data = (cmd_dis_connect*)malloc(sizeof(cmd_dis_connect));
		if (data) {
			data->_type = CMD_END_SESSION;
			data->_sid = sid;
		}
		return (char*)data;
	}
};


#endif