#include "types.h"

#ifdef __linux
;
#else
void perror(const char* str) {
	printf("%s", str);
}
#endif // __linux

namespace CMD {
	char* MAKE_cmd_dis_connect(int sid) {
		cmd_dis_connect* data = NULL;
		data = (cmd_dis_connect*)malloc(sizeof(cmd_dis_connect));
		
		if (data) {
			data->_sid = sid;
		}
		return (char*)data;
	}

    char* MAKE_cmd_login(us16 passwd_size, char* passwd, us16 des_size, char* des) {
        cmd_login* data = NULL;
        data = (cmd_login*)malloc(sizeof(cmd_login));
        if(data){
            assert(passwd_size <= sizeof(data->_passwd));
            assert(des_size <= sizeof(data->_describe));
            
            memcpy(data->_passwd, passwd, sizeof(data->_passwd));
            memcpy(data->_describe, des, sizeof(data->_describe));
        }
        return (char*)data;
    }
};
