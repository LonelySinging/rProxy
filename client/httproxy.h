#ifndef __HTTPROXY_H
#define __HTTPROXY_H

#include "../poll/rpoll.h"
#include "../common/types.h"
#include "client.h"

#include <iostream>
#include <string>
#include <mutex>

using namespace std;

class HttpProxy;
class ServerConn;
class HandleHttp : public GNET::Active{
private:
	ServerConn* _server_conn;
	int _sid;
	char* _buff;
public:
	HandleHttp(string host, int port, ServerConn* server_conn, int sid) : GNET::Active(host, port),
		_server_conn(server_conn),
		_sid(sid),
		_buff(NULL){};
	~HandleHttp() {
		_server_conn = NULL;
		_sid = 0;
		if (_buff) {
			free(_buff);
			_buff = NULL;
		}
	}
	void OnRecv();
};

class ServerConn;
class HttpProxy {
private:
	bool _is_https;
	static mutex _mtx;
	HandleHttp* _http_handler;
	us16 _sid;
	ServerConn* _server_conn;
	friend class HttpConnecter;
public:
	HttpProxy(us16 sid, ServerConn* server_conn) :
		_is_https(false),
		_sid(sid),
		_http_handler(NULL),
		_server_conn(server_conn) {}

	~HttpProxy() {	// 当 HttpProxy被删除的时候_http_handler也就失去意义了 所以可以在析构函数这里删除
		if (_http_handler) {
			GNET::Poll::deregister_poll(_http_handler, _sid);
			_http_handler->OnClose();
			delete _http_handler;
			_http_handler = NULL;
		}
		
	}

	// 当来数据的时候会调用它
	void OnRecv(char* data, int len);

	void dump(string str, int len = 35) {
		int str_len = str.length();
		int dump_len = (str_len < len) ? str_len : len;
		printf("[Debug]: dump(dump_len=%d)\n", dump_len);
		for (int i = 0; i < dump_len; i++) {
			printf("%c", str.c_str()[i]);
		}
		printf("\n");
	}

	// 通过域名获取IP 需要加锁,因为gethostbyname中保存结果的结构应该是个static的数组
	static bool GetIpByName(const char* str, char ip[]){
		lock_guard<mutex> l(_mtx);
		struct hostent* host = gethostbyname(str);
		if (!host){
			return false;
		}
		sprintf_s(ip, 18, "%s", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
		return true;
	}

};

#endif