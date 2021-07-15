#include "main.h"

using namespace std;

// 1. 如果连接特别多，sid可能会冲突 
// 2. 取消注册 poll √
// 3. 考虑服务端直接断开会发生什么 让客户端考虑去吧

void RequestHandle::OnRecv(){
    char buff[Packet::DATA_SIZE];
    int len = Recv(buff, Packet::DATA_SIZE);
    printf("[Debug]: <-- Request %d\n", len);
    if (len == 0){
        _client_bn->del_session(_sid);
        return ;
    }
    char* send_data = (new Packet(_sid, len, buff))->get_p();
    if (send_data){
        _client_bn->Send(buff, len);
    }else{
        printf("[Warning]: 组装数据包失败 buff=%p, _sid=%d, len=%d\n", buff, _sid, len);
    }
}

void ClientListener::OnRecv(){
    BaseNet* bn = Accept();
    if (bn){
        int sid = get_sid();
        RequestHandle* rh = new RequestHandle(*bn, _client_bn, sid);
        _client_bn->add_session(sid, rh);
        GNET::Poll::register_poll(rh);
        delete bn;  // 删除了结构 但是并没有关闭套接字
    }else{
        printf("[Error]: 与请求端建立连接失败\n");
    }
}

void ClientHandle::OnRecv(){
    char buff[Packet::DATA_SIZE];
    int len = Recv(buff, Packet::DATA_SIZE);
    printf("[Debug]: <-- Client %d\n", len);
    if (len == 0){
        _cl->OnClose();     // 关闭端口监听
        del_session(-1);    // 杀掉孩子们
        OnClose();          // 再关闭自己
        if(_cl){delete _cl;}         // 弑父
        delete this;        // 自杀
        return ;
    }
    Packet* pk = new Packet(buff);
    
    GNET::BaseNet* bn = fetch_bn(pk->get_sid());
    if(bn){
        bn->Send(buff, len);
    }else{
        printf("[Warning]: 没有找到 sid=%d 的会话, len=%d\n", pk->get_sid(), len);
    }
}

void ServerListener::OnRecv(){
	printf("[Info]: 有客户端连接到达\n");
    GNET::BaseNet* bn = Accept();
    if(!bn){
        printf("[Error]: 与客户端建立连接失败\n");
        return ;
    }
    ClientHandle *ch = new ClientHandle(*bn);
    delete bn;

    GNET::Poll::register_poll(ch); // 注册客户端

    printf("[Info]: %s:%d, sock_fd: %d\n", ch->get_host().c_str(), ch->get_port(), ch->get_sock());
    if (_client_count >= CLIENT_COUNT){  // 超过最大数量了
        printf("[Warning]: 客户端连接到上限了 %d\n", _client_count);
        // bn->Send(); // 发送拒绝原因
        ch->OnClose();
        delete ch;
        return ;
    }

    if (_client_port > MAX_PORT){   // 超过最大端口限制
        printf("[Warning]: 端口号超过最大限制 port: %d, MAX_PORT: %d\n",_client_port-1, MAX_PORT);
        _client_port = START_PORT;
    }
    
    int try_count = 0;
    do{
        if(try_count > MAX_TRY_NUM){
            printf("[Warning]: 尝试多次都失败了\n");
            // bn->Send(); // 尝试很多次都失败了
            ch->OnClose();
            delete ch;
            return ;
        }
        try_count ++;
        
        ClientListener* cl = new ClientListener(_host, _client_port, ch);
        if (cl->IsError()){
            cl->OnClose();
            delete cl;
            _client_port ++;
        }else{  // 监听成功
            GNET::Poll::register_poll(cl);
            ch->set_client_listener(cl);
            break;
        }

        if (_client_port > MAX_PORT){
            printf("[Warning]: 端口号超过最大限制 port: %d, MAX_PORT: %d\n",_client_port-1, MAX_PORT);
            _client_count = START_PORT;
        }

    }while(1);

    _client_port++;     // 出了循环表示监听成功
    dec_client_count(); // 客户端计数+1
};

int ServerListener::_client_count = 0;

int main(){
	
    if((new ServerListener("0.0.0.0", 7200))->IsError()){
        return -1;
    }
    
    GNET::Poll::loop_poll();
	/*THREAD::ThreadPool::start_pool();
	
	for (int i=0;i<100;i++){
		while(1){
			if(THREAD::ThreadPool::add_task(new print(i)) == 0){
				break;
			}
		}
		// usleep(100000);
	}*/

	getchar();
}
