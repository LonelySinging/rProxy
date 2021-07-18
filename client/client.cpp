#include <iostream>
#include <stdlib.h>


#include "client.h"

using namespace std;

void ServerConn::OnRecv() {
	printf("[Debug]: OnRecv()\n");
	char* data = (char*)malloc(Packet::PACKET_SIZE);
	int ret = RecvPacket(data, Packet::PACKET_SIZE);
	if (ret == 0) {
		OnClose();
		GNET::Poll::deregister_poll(this);
		GNET::Poll::stop_poll();
		delete this;
		return;
	}
	if (ret == -1) { return; };
	Packet* pk = new Packet(data, ret);
	printf("[Debug]: pk(sid=%d, data_len=%d, str=%s)\n", pk->get_sid(), pk->get_data_len(), pk->get_data());
	pk->dump();
	unsigned short int sid = pk->get_sid();

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

void ServerConn::remove_hp(int sid) {
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
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	GNET::Poll::init_select();
	//if ((new ServerConn("127.0.0.1", 7200))->IsError()) {
	if ((new ServerConn("39.106.164.33", 7200))->IsError()) {
		printf("[Error]: 连接服务器失败 \n");
		return -1;
	}

	GNET::Poll::loop_poll();
	return 0;
}
