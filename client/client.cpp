#include <iostream>
#include <stdlib.h>

#include "../poll/rpoll.h"
#include "../common/packet.h"
#pragma execution_character_set("utf-8")
using namespace std;


class ServerConn : public GNET::Active {
public:
	ServerConn(string host, int port) : Active(host, port) {
		GNET::Poll::register_poll(this);
	};
	~ServerConn() {};

	void OnRecv() {
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

		SendPacket(pk->get_p(), pk->get_packet_len());
		
		free(data);
		delete pk;
	}
};

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
