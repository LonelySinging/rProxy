#include "client.h"
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
		delete this;
		return;
	}
	if (ret == -1) {	// 不完整的数据包
		return;
	}
	Packet* pk = new Packet(_buff, ret);
	us16 sid = pk->get_sid();

	printf("[Debug]: <-- Server %d [%d]\n", ret, sid);
	assert(sid <= MAX_SID && sid >= 0);

	if (sid <= MAX_SID && sid >= 0) {	// sid不合理的话，放弃这个数据包
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
			if (has_hp(sid)) {
				fetch_hp(sid)->OnRecv(pk->get_data(), pk->get_data_len());
			}
			else {
				// 当客户端一个会话结束之后，服务端又发来一个sid，依旧被添加进来了。
				// 这个问题不用担心，会因为_http_handler==NULL而在OnRecv()中被移除，但确实多此一举了。
				// 考虑把remove_hp()操作放在一个地方完成。比如只会在接收到服务端结束session命令的时候才删除。
				// 如果是直接与服务端断开的话，当然还是要remove_hp(-1);以免内存泄漏
				add_hp(sid, new HttpProxy(sid, this));
				fetch_hp(sid)->OnRecv(pk->get_data(), pk->get_data_len());
			}
		}
	}
	delete pk;
}

void ServerConn::send_cmd(char* data, int len) {
	if (!data) { return; }
	if (SendPacket(data, len, true) <= 0) {
		// OnClose();
	}
	free(data);
}

void ServerConn::OnClose() {	// 重写父类方法的话，也需要实现原有 OnClose()的功能
	GNET::BaseNet::OnClose();
	remove_hp(-1);
}

void ServerConn::remove_hp(int sid) {
	lock_guard<mutex> l(_hps_mtx);
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
	printf("用法: \n\tclient -h IP地址 -p 端口号 -n 备注 -t 口令\n\t例子: client -h 39.106.164.33 -p 7200 -n win11 -t 123456\n\n");
}

int main(int argv, char* args[]) {
	// 因为linux默认是utf-8编码，所以代码文件是utf-8，以至于字符串常量编码也是
	// 但是windows的控制台编码是gbk，所以会出现乱码问题
	system("chcp 65001 && cls");	
	char* host = "39.106.164.33";
	int port = 7201;
	char* note = NULL;		// 备注
	char* token = NULL;		// 登录口令

	if (argv == 1) {
		usage();
	}else {
		for (int i = 1; i < argv; i++) {
			if (args[i][0] == '-' || args[i][0] == '/') {	// 认为是命令
				if (argv > (i+1)) {	// 合法范围
					switch (args[i][1]){
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
						token = args[i + 1];		// 因为控制台是gbk编码，所以中文传给服务端会乱码
						break;
					default:
						break;
					}
					i++;	// 跳过参数内容
				}
			}
		}
	}
	if (!token) {
		if (strlen(token) > CMD::cmd_login::PASSWD_LEN) {
			printf("[Error]: 口令长度不能大于%d\n", CMD::cmd_login::PASSWD_LEN);
			return -2;
		}
	}

	if (!note) {
		if (strlen(note) > CMD::cmd_login::DES_LEN) {
			printf("[Error]: 描述长度不能大于%d\n", CMD::cmd_login::DES_LEN);
			return -3;
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
