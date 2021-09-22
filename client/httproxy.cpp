#include "httproxy.h"
#include "../common/httpheader.h"
#include "../common/types.h"
#include "../thread/rthread.h"

using std::string;

mutex HttpProxy::_mtx;

class HttpConnecter : public THREAD::Runnable{
	/*
		用于把连接和传输分离开来，建立连接的时候通过线程完成，免得出现比较慢的连接过程导致客户端阻塞
		如果使用线程池的话，极端情况下每一个连接都很慢的时候，最终还是会把线程池占满
	*/
private:
	string _http_str;			// 原始请求
	ServerConn* _server_conn;	// 失败了的话 需要他删除会话
	HttpProxy* _http_proxy;
	int _sid;					// 创建 HandleHttp 对象所必须的

public:
	HttpConnecter(string http_str, HttpProxy* http_proxy, int sid, ServerConn* server_conn) :
			_http_str(http_str),
			_server_conn(server_conn),
			_http_proxy(http_proxy),
			_sid(sid){}

	void run() {
		do {
			if (_http_str.find("GET") == 0 || _http_str.find("POST") == 0 || _http_str.find("HEAD") == 0) {
				HttpHeader hh(_http_str);
				char ip[128] = { 0 };
				int port = hh.get_port();
				if (HttpProxy::GetIpByName(hh.get_host().c_str(), ip) && port) {
					_http_proxy->_http_handler = new HandleHttp(ip, port, _server_conn, _sid);	// 与http服务端建立连接
					if (!_http_proxy->_http_handler->IsError()) {
						string new_hh = hh.rewrite_header();
						if (_http_proxy->_http_handler->SendN((char*)new_hh.c_str(), new_hh.length()) <= 0) {
							printf("[Error]: 发送到http失败 sid=[%d]\n", _sid);
						}else {
							GNET::Poll::register_poll(_http_proxy->_http_handler);
							break;
						}
					}else {
						printf("[Debug]: 连接http失败 %s:%d [%d]\n", ip, port, _sid);
					}
				}else {
					printf("[Error]: 域名解析失败 %s:%d [%d]\n", hh.get_host().c_str(), hh.get_port(), _sid);
				}
				printf("[Info]: 结束会话 [%d]\n", _sid);
				_server_conn->remove_hp(_sid);
			}
			else if (_http_str.find("CONNECT") == 0) {	// https 请求
				HttpHeader hh(_http_str);
				char ip[128] = { 0 };
				int port = hh.get_port();
				if (HttpProxy::GetIpByName(hh.get_host().c_str(), ip) && port) {
					_http_proxy->_http_handler = new HandleHttp(ip, port, _server_conn, _sid);	// 与https服务端建立连接
					if (!_http_proxy->_http_handler->IsError()) {
						Packet* pk = new Packet(_sid, strlen("HTTP/1.1 200 Connection established\r\n\r\n"), "HTTP/1.1 200 Connection established\r\n\r\n");
						int ret = _server_conn->SendPacket(pk->get_p(), pk->get_packet_len(), true);
						printf("[Debug]: 发送认证 %d [%d]\n", ret, _sid);
						delete pk;
						if (ret <= 0) {
							printf("[Error]: 发送到Server失败 sid=[%d]\n", _sid);
							_server_conn->OnClose();										// 与服务端断开了 开始收尸吧
						}else {
							GNET::Poll::register_poll(_http_proxy->_http_handler);
							break;
						}
					}else {
						printf("[Debug]: 连接http失败 %s:%d [%d]\n", ip, port, _sid);
					}
				}else {
					printf("[Error]: 域名解析失败 %s:%d [%d]\n", hh.get_host().c_str(), hh.get_port(), _sid);
				}
				printf("[Info]: 结束会话 %d\n", _sid);
				_server_conn->remove_hp(_sid);	// 暂时不处理 https
			}
			else { assert(false && "怎么想都执行不到这里吧？？？"); }
		} while (false);	// 是为了配合break
		set_delete();
	}
};

void HandleHttp::OnRecv() {
	char* buff = (char*)malloc(Packet::DATA_SIZE);	// 可以作为一个常备缓冲区
	int ret = Recv(buff, Packet::DATA_SIZE);
	if (ret <= 0) {
		printf("[Debug]: 接收结束 ret=%d sid=[%d]\n", ret, _sid);
		_server_conn->remove_hp(_sid);	// 结束 handle
	}else {
		printf("[Debug]: <-- Http %d [%d]\n", ret, _sid);
		Packet* pk = new Packet(_sid, ret, buff);
		assert(ret == pk->get_data_len());
		assert(_sid == pk->get_sid());
		ret = _server_conn->SendPacket(pk->get_p(), pk->get_packet_len(), true);
		printf("[Debug]: --> Server %d [%d] plen %d\n", ret, pk->get_sid(), (int)pk->get_packet_len());
		assert(pk->get_packet_len() <= Packet::PACKET_SIZE);
		//assert(pk->get_packet_len() == (ret-2));
		delete pk;
	}
	free(buff);
}

// 参数是不带sid头的数据
void HttpProxy::OnRecv(char* data, int len) {
	string http_str(data, len);
	// http 请求
	if (http_str.find("GET") == 0 || http_str.find("POST") == 0 || http_str.find("HEAD") == 0 || http_str.find("CONNECT") == 0) {
		HttpConnecter* hc = new HttpConnecter(http_str, this, _sid, _server_conn);	// run 完毕之后删除自己
		THREAD::ThreadHelper::start_thread_task(hc);
	}else {
		if (!_http_handler) {
			printf("[Error]: 收到了错误的请求\n");
			dump(http_str);
			// OnClose();
			_server_conn->remove_hp(_sid);
		}
		else {
			//Packet* pk = new Packet(_sid, len, data);
			len = _http_handler->SendN(data, len);
			//delete pk;
			printf("[Debug]: --> Http %d [%d]\n", len, _sid);
		}
	}
}

void HttpProxy::OnClose() {
	if (_http_handler) {
		GNET::Poll::deregister_poll(_http_handler);
		_http_handler->OnClose();
		delete _http_handler;
		_http_handler = NULL;
	}
	_server_conn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));	// 通知服务端这个会话已经结束了
};