#ifndef __CLIENT_H
#define __CLIENT_H

#include "../poll/rpoll.h"
#include "../common/packet.h"
#include "httproxy.h"

#include <map>

using namespace std;

class HttpProxy;
class ServerConn : public GNET::Active {
private:
	map<int, HttpProxy*> _hps;
	char* _buff;
public:
	ServerConn(string host, int port) : Active(host, port) {
		GNET::Poll::register_poll(this);
		_buff = (char*)malloc(4098);	// 暂不考虑通用性问题 因为全局只有这么一个对象，所以暂时不考虑拷贝构造函数
	};
	~ServerConn() {
		if (_buff) {
			free(_buff);
			_buff = NULL;
		}
	};

	void OnClose();
	void OnRecv();
	void send_cmd(char* data, int len);	// 发送控制指令

	void remove_hp(int sid);
};

#endif