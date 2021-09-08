#include "client.h"
#include "httpheader.h"
#include "../common/types.h"


#include <iostream>
#include <stdlib.h>

using namespace std;

void ServerConn::OnRecv() {
	int ret = RecvPacket(_buff, Packet::PACKET_SIZE);
	if (ret == 0) {
		OnClose();
		GNET::Poll::deregister_poll(this);
		GNET::Poll::stop_poll();
		printf("[Info]: 与服务器断开 ret=[%d]\n", ret);
		// 还需要关闭与http服务器的连接
		delete this;
		return;
	}
	if (ret == -1) {	// 不完整的数据包
		return;
	}
	Packet* pk = new Packet(_buff, ret);

	us16 sid = pk->get_sid();

	printf("[Debug]: <-- Server %d [%d]\n", ret, sid);
	assert(sid <= MAX_SID && sid >= MIX_SID);

	if (sid == 0) {		// 这是个控制指令
		switch (((CMD::cmd_dis_connect*)pk->get_p())->_type) {
		case CMD::CMD_END_SESSION:
		{
			int s = ((CMD::cmd_dis_connect*)pk->get_p())->_sid;
			remove_hp(s);
			break;
		}
		default:
			break;
		}
	}
	else {
		map<int, HttpProxy*>::iterator iter = _hps.find(sid);
		if (iter == _hps.end()) {
			_hps[sid] = new HttpProxy(sid, this);
			_hps[sid]->OnRecv(pk->get_data(), pk->get_data_len());
		}
		else {
			_hps[sid]->OnRecv(pk->get_data(), pk->get_data_len());
		}
	}
	delete pk;
}

void ServerConn::send_cmd(char* data, int len) {
	if (!data) { return; }
	if (SendPacket(data, len) <= 0) {
		// OnClose();
	}
	free(data);
}

void ServerConn::OnClose() {	// 重写父类方法的话，也需要实现原有 OnClose()的功能
	GNET::BaseNet::OnClose();	// 好像这样就能调用父类方法了？
	// 需要在此处处理与http服务器的连接 如果想要实现断线重连的话。。。应该会在main()中实现
	remove_hp(-1);
}

void ServerConn::remove_hp(int sid) {
	if (sid == -1) {
		map<int, HttpProxy*>::iterator iter = _hps.begin();	// 这地方不能使用递归删除，因为调用自己删除之后 这里的循环中的迭代器就失效了
		for (; iter != _hps.end(); iter++) {
			iter->second->OnClose();
			delete iter->second;
		}
		_hps.clear();
		return;
	}
	map<int, HttpProxy*>::iterator iter = _hps.find(sid);
	if (iter != _hps.end()) {
		_hps[sid]->OnClose();
		delete _hps[sid];
		_hps.erase(iter);
	}
}

void usage() {
	printf("用法: \n\tclient -h IP地址 -p 端口号 -n 备注 -t 口令\n\t例子: client -h 39.106.164.33 -p 7200 -n 家里的win11 -t 123456\n\n");
}

int main(int argv, char* args[]) {
	system("chcp 65001 && cls");
	char* host = "39.106.164.33";
	int port = 7200;
	char* note = NULL;
	char* token = NULL;

	if (argv == 1) {
		usage();
	}
	else {
		for (int i = 1; i < argv; i++) {
			if (args[i][0] == '-' || args[i][0] == '/') {	// 认为是命令
				if (argv > (i+1)) {	// 合法范围
					switch (args[i][1])
					{
					case 'h':
						host = args[i + 1];
						break;
					case 'p':
						port = atoi(args[i+1]);
						break;
					case 'n':
						note = args[i + 1];
						break;
					case 't':
						token = args[i + 1];
						break;
					default:
						break;
					}
					i++;	// 跳过参数内容
				}
			}
		}
	}

	// windows 特色...
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	GNET::Poll::init();
	//if ((new ServerConn("127.0.0.1", 7200))->IsError()) {
	if ((new ServerConn(host, port, note, token))->IsError()) {
		printf("[Error]: 连接服务器失败 \n");
		return -1;
	}

	GNET::Poll::loop_poll();
	getchar();
	return 0;
}
