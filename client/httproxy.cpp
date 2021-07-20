#include "httproxy.h"
#include "httpheader.h"

using std::string;

void HandleHttp::OnRecv() {
	char* buff = (char*)malloc(Packet::DATA_SIZE);	// 可以作为一个常备缓冲区
	int ret = Recv(buff, Packet::DATA_SIZE);
	if (ret <= 0) {
		OnClose();	// 关闭与http的联系
		_server_conn->remove_hp(_sid);	// 结束 handle
	}
	Packet* pk =  new Packet(_sid, ret, buff);
	free(buff);
	_server_conn->SendPacket(pk->get_p(), pk->get_packet_len());
	delete pk;
}

// 参数是不带sid头的数据
void HttpProxy::OnRecv(char* data, int len) {
	string http_str(data, len);
	// http 请求
	
	if (http_str.find("GET") == 0 || http_str.find("POST") == 0 || http_str.find("HEAD") == 0) {
		// printf("[Debug]: 是 http 请求\n");
		HttpHeader hh(http_str);
		char ip[128] = {0};
		;
		int port = hh.get_port();
		if (GetIpByName(hh.get_host().c_str(), ip) && port) {
			_http_handler = new HandleHttp(ip, port, _server_conn, _sid);	// 与http服务端建立连接
			if (!_http_handler->IsError()) {
				_http_handler->Send((char*)hh.rewrite_header().c_str(), http_str.length());
				GNET::Poll::register_poll(_http_handler);
				return;
			}
		}
		else {
			printf("[Error]: 域名解析失败 %s:%d\n", hh.get_host().c_str(), hh.get_port());
		}
		printf("[Info]: 结束会话 %d\n", _sid);
		OnClose();
		_server_conn->remove_hp(_sid);
	}
	else if (http_str.find("CONNECT") == 0) {	// https 请求
		_server_conn->remove_hp(_sid);	// 暂时不处理 https
	}
	else {
		if (!_http_handler) {
			printf("[Error]: 收到了错误的请求\n");
			dump(http_str);
			OnClose();
			_server_conn->remove_hp(_sid);
		}
		else {
			_http_handler->Send(data, len);
		}
	}
}