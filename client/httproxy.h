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
public:
	HandleHttp(string host, int port, ServerConn* server_conn, int sid) : GNET::Active(host, port),
		_server_conn(server_conn),
		_sid(sid){};
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

	~HttpProxy() {}

	// 当来数据的时候会调用它
	void OnRecv(char* data, int len);
	void OnClose();

	void dump(string str, int len = 35) {
		int str_len = str.length();
		int dump_len = (str_len < len) ? str_len : len;
		printf("[Debug]: dump(dump_len=%d)\n", dump_len);
		for (int i = 0; i < dump_len; i++) {
			printf("%c", str.c_str()[i]);
		}
		printf("\n");
	}

	// 通过域名获取IP 恐怕得加锁
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