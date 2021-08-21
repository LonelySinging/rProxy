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
			data->_raw_sid = 0;
			data->_type = CMD_END_SESSION;
			data->_sid = sid;
		}
		return (char*)data;
	}
};