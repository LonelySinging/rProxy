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
		int str_len = http_str.length();
		int header_end = http_str.find("\r\n\r\n");	// 请求头的尾部 所有操作不该超过头

		int host_pos = http_str.find("Host: ");
		int host_line_end = http_str.find("\r\n", host_pos);	// host行尾部
		// printf("[Debug]: host_pos: %d, host_line_end: %d\n", host_pos, host_line_end);
		if (host_pos < header_end && host_line_end < header_end) {
			int http_pos = http_str.find("http://");
			string host_ip = http_str.substr(host_pos + strlen("Host: "), host_line_end);

			if (host_ip != "") {
				string host;
				int port = 0;
				if (host_ip.find(":") < host_ip.length()) {	// 指定了端口
					host = host_ip.substr(0, host_ip.find(":"));
					//printf("[Debug]: host: %s\n", host.c_str());
					string port_str = host_ip.substr(host_ip.find(":") + 1, host_line_end).c_str();
					port = atoi(port_str.c_str());
					//printf("[Debug]: port: %d\n", port);
				}
				else {
					host = host_ip;
					port = 80;
				}
				
				printf("[Debug]: str_len=%d, header_end=%d, host_pos=%d, host_line_end=%d\
							host=%s",
					str_len, header_end, host_pos, host_line_end, host.c_str());
				//dump(http_str, header_end);
				char* ip_str = GetIpByName(host.c_str());
				if (!ip_str) {
					printf("[Debug]: 解析域名错误 %s:%d, errcode: %d\n",host.c_str(), port, GetLastError());
				}
				else {
					http_str.erase(http_pos, strlen("http://") + host_ip.length());
					string ip(ip_str);
					printf("[info]: 解析ip: %s:%d\n", ip.c_str(), port);
					_http_handler = new HandleHttp(ip, port, _server_conn, _sid);	// 与http服务端建立连接
					if (!_http_handler->IsError()) {
						_http_handler->Send((char*)http_str.c_str(), http_str.length());
						GNET::Poll::register_poll(_http_handler);
						return;
					}
				}
			}
			else {
				printf("[Warning]: 没有找到host\n");
			}
		}
		else {
			printf("[Warning]: 错误的包结构\n");
		}
		printf("[Info]: 结束会话 %d\n", _sid);
		dump(http_str, header_end);
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