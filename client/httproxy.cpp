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
		set_delete();	// 因为是通过标志位由ThreadHelper代劳删除,所以一开始就设置标志位是没有问题的
		if (_http_proxy->_http_handler) {
			printf("[Debug]: 已经建立链接的http又一次建立链接 sid: %d\n", _sid);	// 愚蠢的浏览器可能会发送两次请求，但愿这么处理不会让它发疯
			return;
		}
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
						GNET::Poll::register_poll(_http_proxy->_http_handler, _sid);
						_http_proxy->set_forbid_delete(false);
						return;
					}
				}else {
					printf("[Debug]: 连接http失败 %s:%d [%d]\n", ip, port, _sid);
				}
			}else {
				printf("[Error]: 域名解析失败 %s:%d [%d]\n", hh.get_host().c_str(), hh.get_port(), _sid);
			}
			printf("[Info]: 结束会话 [%d]\n", _sid);
		}
		else if (_http_str.find("CONNECT") == 0) {	// https 请求
			HttpHeader hh(_http_str);
			char ip[128] = { 0 };
			int port = hh.get_port();
			if (HttpProxy::GetIpByName(hh.get_host().c_str(), ip) && port) {
				_http_proxy->_http_handler = new HandleHttp(ip, port, _server_conn, _sid);	// 与https服务端建立连接
				if (!_http_proxy->_http_handler->IsError()) {
					Packet pk(_sid, strlen("HTTP/1.1 200 Connection established\r\n\r\n"), "HTTP/1.1 200 Connection established\r\n\r\n");
					int ret = _server_conn->SendPacket(pk.get_p(), pk.get_packet_len(), true);
					// printf("[Debug]: 发送认证 %d [%d]\n", ret, _sid);
					if (ret <= 0) {
						printf("[Error]: 发送到Server失败 sid=[%d]\n", _sid);
					}else {
						GNET::Poll::register_poll(_http_proxy->_http_handler, _sid);
						_http_proxy->set_forbid_delete(false);
						return;
					}
				}else {
					printf("[Debug]: 连接http失败 %s:%d [%d]\n", ip, port, _sid);
				}
			}else {
				printf("[Error]: 域名解析失败 %s:%d [%d]\n", hh.get_host().c_str(), hh.get_port(), _sid);
			}
			printf("[Info]: 结束会话 %d\n", _sid);
		}
		else { assert(false && "怎么想都执行不到这里吧？？？"); }
		_http_proxy->set_forbid_delete(false);
		// _server_conn->remove_hp(_sid);
		_server_conn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));
	}
};

void HandleHttp::OnRecv() {
	if (!_buff) {
		_buff = (char*)malloc(Packet::DATA_SIZE);
	}
	int ret = Recv(_buff, Packet::DATA_SIZE);
	if (ret <= 0) {
		printf("[Debug]: 接收结束 ret=%d sid=[%d]\n", ret, _sid);
		// _server_conn->remove_hp(_sid);	// 结束 handle
		_server_conn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));
		GNET::Poll::deregister_poll(this);
	}else {
		// printf("[Debug]: <-- Http %d [%d]\n", ret, _sid);
		Packet pk(_sid, ret, _buff);
		assert(ret == pk.get_data_len());
		assert(_sid == pk.get_sid());
		ret = _server_conn->SendPacket(pk.get_p(), pk.get_packet_len(), true);
		// printf("[Debug]: --> Server %d [%d] plen %d\n", ret, pk.get_sid(), (int)pk.get_packet_len());
		assert(pk.get_packet_len() <= Packet::PACKET_SIZE);
	}
}

// 参数是不带sid头的数据
void HttpProxy::OnRecv(char* data, int len) {
	string http_str(data, len);
	if (http_str.find("GET") == 0 || http_str.find("POST") == 0 || http_str.find("HEAD") == 0 || http_str.find("CONNECT") == 0) {
		// 这里不能使用局部变量完成 当走出这个代码块之后，HttpConnecter成员变量会被释放
		// 因为建立连接之前，不管是客户端还是服务端都不会再发送数据过来，所以不会出现线程还没建立连接就来了新数据而_http_handler==NULL
		// 导致关闭会话的问题。但是如果首次接收的消息不完整，那确实有可能出错 如果出现一个请求被分包成了两个 若是包含了足够的建立连接的信息
		// 会导致后半个包被发送给http服务器导致http断开连接
		set_forbid_delete();	// 防止在链接期间这个对象被删除
		HttpConnecter* hc = new HttpConnecter(http_str, this, _sid, _server_conn);	// run 完毕之后删除自己
		THREAD::ThreadHelper::start_thread_task(hc);	// 直接交给一个独立线程去完成
	}else {
		if (!_http_handler) {
			printf("[Error]: 收到了错误的请求 %d\n", _sid);
			dump(http_str);
			// _server_conn->remove_hp(_sid);
			_server_conn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));
		}
		else {
			len = _http_handler->SendN(data, len);
			// printf("[Debug]: --> Http %d [%d]\n", len, _sid);
		}
	}
}
