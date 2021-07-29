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
public:
	ServerConn(string host, int port) : Active(host, port) {
		GNET::Poll::register_poll(this);
	};
	~ServerConn() {};

	void OnClose();
	void OnRecv();
	void send_cmd(char* data, int len);	// ·¢ËÍ¿ØÖÆÖ¸Áî

	void remove_hp(int sid);
};

#endif