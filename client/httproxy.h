#ifndef __HTTPROXY_H
#define __HTTPROXY_H

#include "../poll/rpoll.h"
#include "client.h"
#include <iostream>
#include <string>

using std::string;

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
	HandleHttp* _http_handler;
	unsigned short int _sid;
	ServerConn* _server_conn;

public:
	HttpProxy(unsigned short int sid, ServerConn* server_conn) :
		_is_https(false),
		_sid(sid),
		_http_handler(NULL),
		_server_conn(server_conn) {}

	~HttpProxy() {}

	// 当来数据的时候会调用它
	void OnRecv(char* data, int len);
	void OnClose() {
		if (_http_handler) {
			GNET::Poll::deregister_poll(_http_handler);
			_http_handler->OnClose();
		}
		_server_conn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));	// 通知服务端这个会话已经结束了
	};

	void dump(string str, int len = 35) {
		int str_len = str.length();
		int dump_len = (str_len < len) ? str_len : len;
		printf("[Debug]: dump(dump_len=%d)\n", dump_len);
		for (int i = 0; i < dump_len; i++) {
			printf("%c ", str.c_str()[i]);
		}
		printf("\n");
	}

	// 通过域名获取IP
	bool GetIpByName(const char* str, char ip[])
	{
		struct hostent* host = gethostbyname(str);
		if (!host){
			return false;
		}
		sprintf_s(ip, 18, "%s", inet_ntoa(*(struct in_addr*)host->h_addr_list[0]));
		return true;
	}

};

#endif