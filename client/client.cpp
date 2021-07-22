#include <iostream>
#include <stdlib.h>


#include "client.h"
#include "httpheader.h"

using namespace std;

void ServerConn::OnRecv() {
	printf("[Debug]: OnRecv()\n");
	char* data = (char*)malloc(Packet::PACKET_SIZE);
	int ret = RecvPacket(data, Packet::PACKET_SIZE);
	if (ret == 0) {
		OnClose();
		GNET::Poll::deregister_poll(this);
		GNET::Poll::stop_poll();
		printf("[Info]: 与服务器断开 ret=%d\n", ret);
		// 还需要关闭与http服务器的连接
		delete this;
		return;
	}
	if (ret == -1) {	// 不完整的数据包
		return;
	}
	Packet* pk = new Packet(data, ret);
	//printf("[Debug]: pk(sid=%d, data_len=%d, str=%s)\n", pk->get_sid(), pk->get_data_len(), pk->get_data());
	//pk->dump();
	unsigned short int sid = pk->get_sid();

	printf("[Debug]: <-- Server %d\n", ret);

	map<int, HttpProxy*>::iterator iter = _hps.find(sid);
	if (iter == _hps.end()) {
		_hps[sid] = new HttpProxy(sid, this);
		_hps[sid]->OnRecv(pk->get_data(), pk->get_data_len());
	}
	else {
		_hps[sid]->OnRecv(pk->get_data(), pk->get_data_len());
	}

	free(data);
	delete pk;
}

void ServerConn::OnClose() {	// 重写父类方法的话，也需要实现原有 OnClose()的功能
#ifdef __linux
	close(_sock_fd);
#else
	closesocket(_sock_fd);
#endif
	// 需要在此处处理与http服务器的连接 如果想要实现短线重连的话。。。应该会在main()中实现
	remove_hp(-1);
}

void ServerConn::remove_hp(int sid) {
	if (sid == -1) {
		map<int, HttpProxy*>::iterator iter = _hps.begin();	// 这地方不能使用递归删除，因为调用自己删除之后 这里的循环中的迭代器就失效了
		for (; iter != _hps.end(); iter++) {
			iter->second->OnClose();
			delete iter->second;
			_hps.erase(iter);
			iter = _hps.begin();
		}
		return;
	}
	map<int, HttpProxy*>::iterator iter = _hps.find(sid);
	if (iter != _hps.end()) {
		_hps[sid]->OnClose();
		delete _hps[sid];
		_hps.erase(iter);
	}
}

int main() {
	system("chcp 65001 && cls");
	// windows 特色...
/*
		// HttpHeader调试
	//char http_str[] = "GET http://www.baidu.com/hh?gg=3&uu=4 HTTP/1.1\r\nHost: www.baidu.com\r\nAccept: text*uu\r\nUA: Chrome\r\n\r\nbody";
	char http_str[] = "GET http://www.baidu.com:8080/hh?gg=3&uu=4 HTTP/1.1\r\nHost: www.baidu.com:8080\r\nAccept: text*uu\r\nUA: Chrome\r\n\r\nbody";
	//char http_str[] = "CONNECT baidu.com:443 HTTP/1.0\r\n\r\n";
	HttpHeader* hh = new HttpHeader(string(http_str));
	cout << "accept: "<< hh->has_key("Host") << endl;
	cout << "host: " << hh->get_host() << endl;
	cout << "port: " << hh->get_port() << endl;
	cout << "reheader: \n" << hh->rewrite_header() << endl;
	getchar();
	exit(-1);*/

	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	GNET::Poll::init_select();
	//if ((new ServerConn("127.0.0.1", 7200))->IsError()) {
	if ((new ServerConn("39.106.164.33", 7200))->IsError()) {
		printf("[Error]: 连接服务器失败 \n");
		return -1;
	}

	GNET::Poll::loop_poll();
	getchar();
	return 0;
}
