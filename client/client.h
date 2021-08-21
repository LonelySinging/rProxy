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
		_buff = (char*)malloc(4098);	// �ݲ�����ͨ�������� ��Ϊȫ��ֻ����ôһ������������ʱ�����ǿ������캯��
	};
	~ServerConn() {
		if (_buff) {
			free(_buff);
			_buff = NULL;
		}
	};

	void OnClose();
	void OnRecv();
	void send_cmd(char* data, int len);	// ���Ϳ���ָ��

	void remove_hp(int sid);
};

#endif