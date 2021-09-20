#ifndef __CLIENT_H
#define __CLIENT_H

#include "../poll/rpoll.h"
#include "../common/packet.h"
#include "../common/types.h"
#include "httproxy.h"

#include <map>
#include <mutex>

using namespace std;

class HttpProxy;
class ServerConn : public GNET::Active {
private:
	map<int, HttpProxy*> _hps;
	mutex _hps_mtx;
	char* _buff;
	char _note[CMD::cmd_login::DES_LEN] = {0};
	char _token[CMD::cmd_login::PASSWD_LEN] = {0};
public:
	ServerConn(string host, int port, char* note, char* token) : Active(host, port) {
		GNET::Poll::register_poll(this);
		_buff = (char*)malloc(Packet::PACKET_SIZE+2);	// 暂不考虑通用性问题 因为全局只有这么一个对象，所以暂时不考虑拷贝构造函数
		if (note)memcpy(_note, note, strlen(note));
		if (token)memcpy(_token, token, strlen(token));
		send_cmd(CMD::MAKE_cmd_login(_token, _note), sizeof(CMD::cmd_login));
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
	void add_hp(int sid, HttpProxy* hp) {
		lock_guard<mutex> l(_hps_mtx);
		_hps[sid] = hp;
	};
	bool has_hp(int sid) {
		lock_guard<mutex> l(_hps_mtx);
		map<int, HttpProxy*>::iterator iter = _hps.find(sid);
		if (iter != _hps.end()) {
			return true;
		}
		return false;
	};
	HttpProxy* fetch_hp(int sid) {
		lock_guard<mutex> l(_hps_mtx);
		map<int, HttpProxy*>::iterator iter = _hps.find(sid);
		if (iter == _hps.end()) { return NULL; }
		return _hps[sid];
	};
};

#endif