#include "main.h"


using namespace std;

// 1. 如果连接特别多，sid可能会冲突 
// 2. 取消注册 poll √
// 3. 考虑服务端直接断开会发生什么 让客户端考虑去吧
// 4. 断开请求端之后，清理了具体对象，但是在poll的队列中，对象指针还在，被访问的话 就会出错

map<int, RunStatus::ClientInfo*> RunStatus::_cis;
char RunStatus::_passwd[CMD::cmd_login::PASSWD_LEN] = "passwd";

void RequestHandle::OnRecv(){
    char buff[Packet::DATA_SIZE];
    int len = Recv(buff, Packet::DATA_SIZE);
    if (len <= 0 || !_client_bn->_active){  // 客户端没有登陆的话，不接受请求端数据
        _client_bn->del_session(_sid);
        return ;
    }
    Packet* pk = new Packet(_sid, len, buff);
    printf("[Debug]: <-- Request %d sid=%d\n", (int)pk->get_packet_len(), pk->get_sid());
    char* send_data = pk->get_p();
    // pk->dump();
    if (send_data){
        int ret = _client_bn->SendPacket(send_data, pk->get_packet_len(), true);
        printf("[Debug]: --> Client %d sid=%d\n", ret, pk->get_sid());
    }else{
        printf("[Warning]: 组装数据包失败 buff=%p, _sid=%d, len=%d\n", buff, _sid, len);
        _client_bn->del_session(_sid);
    }
    delete pk;
}

void RequestHandle::OnClose(){
    GNET::BaseNet::OnClose();
    _client_bn->send_cmd(CMD::MAKE_cmd_dis_connect(_sid), sizeof(CMD::cmd_dis_connect));
    // 如果是ClentHandle因为与客户端断开导致执行这个方法，Send肯定失败 需要考虑嘛
}

// 有请求端请求连接
void ClientListener::OnRecv(){
    BaseNet* bn = Accept();
    if (bn){
        int sid = 0;
        do{
            sid = get_sid();
        }while(_client_bn->fetch_bn(sid));
        RequestHandle* rh = new RequestHandle(*bn, _client_bn, sid);
        _client_bn->add_session(sid, rh);
        GNET::Poll::register_poll(rh);
        delete bn;  // 删除了结构 但是并没有关闭套接字
    }else{
        printf("[Error]: 与请求端建立连接失败\n");
    }
}

void ClientHandle::OnRecv(){
    int len = RecvPacket(_buff, Packet::PACKET_SIZE);
    if (len == 0){
        OnClose();
        return ;
    }
    if (len == -1){ // 不是个完整的数据包 只是接收到了包长信息
        return ;
    }
    Packet* pk = new Packet(_buff, len); // 一个完整的数据包
    printf("[Debug]: <-- Client %d sid=%d\n", len, pk->get_sid());
    assert(pk->get_sid() >= 0 && pk->get_sid() <= ClientListener::MAX_SID);
    if (pk->get_sid() == 0){    // 来自客户端的控制指令
        switch(((CMD::cmd_dis_connect*)pk->get_p())->_type){
        case CMD::CMD_END_SESSION:
            // CMD::cmd_dis_connect* cmd = (CMD::cmd_dis_connect*)pk->get_p();
            if (pk->get_packet_len() != sizeof(CMD::cmd_dis_connect)){OnClose();}
            del_session(((CMD::cmd_dis_connect*)pk->get_p())->_sid);
            // 断开请求端 让他不要发东西了 只管发 收没收到不重要
            break;
        case CMD::CMD_LOGIN:    // 登录
        {
            CMD::cmd_login* cmd = (CMD::cmd_login*)pk->get_p();
            if (pk->get_packet_len() != sizeof(CMD::cmd_login)){OnClose();}
            if (RunStatus::auth_login(cmd->_passwd)){
                RunStatus::ClientInfo* ci = RunStatus::get_client_info(_cl->get_port());
                if (ci){
                    ci->_active = true;
                    _active = true;
                    ci->_client_describe = string(cmd->_describe, CMD::cmd_login::DES_LEN);
                    printf("[Info] 客户端登录成功 [%s]\n", cmd->_describe);
                }else{
                    printf("[Warning] 找不到对应的客户端信息 server_port: %d\n", _cl->get_port());
                    OnClose();
                }
            }else{
                printf("[Info] 客户端登录失败 pass: %s\n", cmd->_passwd);
                OnClose();
            }
            break;
        }
        default:
            break;
        }
    }else{
        if (!_active){OnClose();}   // 没有登陆不允许接收东西
        GNET::BaseNet* bn = fetch_bn(pk->get_sid());
        if(bn){
            // 将来SendN都应该交给线程池去做
            int ret = bn->SendN(pk->get_data(), pk->get_data_len());
            printf("[Debug]: --> Request %d sid=%d\n", ret, pk->get_sid());
        }else{
            printf("[Warning]: 没有找到 sid=%d 的会话, len=%d\n", pk->get_sid(), len);
            send_cmd(CMD::MAKE_cmd_dis_connect(pk->get_sid()), sizeof(CMD::cmd_dis_connect));
        }
    }
    delete pk;
}

void ClientHandle::OnClose(){
    RunStatus::del_client_info(_cl->get_port());
    GNET::BaseNet::OnClose();   // 关闭与客户端的连接
    _cl->OnClose();             // 关闭端口监听
    del_session(-1);            // 断开所有的请求端
    if(_cl){delete _cl;}        // 释放监听对象
    delete this;                // 删除自己 目前没有记录ServerListen，否则应该交给SL去做
    ServerListener::inc_client_count(); 
    // 减少客户端计数 不放在析构函数是因为需要把用到了ServerListener的实现，
    // 就需要把析构函数放在cpp中实现(预定义不行)，emmmm 觉得太麻烦了
}

void ClientHandle::send_cmd(char* data, int len){
    if(SendPacket(data, len) <= 0){
        OnClose();  // 与客户端断开了 处理后事
    }
}

void ServerListener::OnRecv(){
	printf("[Info]: 有客户端连接到达\n");
    GNET::BaseNet* bn = Accept();
    if(!bn){
        printf("[Error]: 与客户端建立连接失败\n");
        return ;
    }
    
    // 因为未来需要向客户端发送错误原因，所以需要现在就包装
    ClientHandle *ch = new ClientHandle(*bn);   
    delete bn;

    printf("[Info]: %s:%d, sock_fd: %d\n", ch->get_host().c_str(), ch->get_port(), ch->get_sock());
    if (_client_count >= CLIENT_COUNT){  // 超过最大数量了
        printf("[Warning]: 客户端连接到上限了 当前客户端数: %d\n", _client_count);
        // bn->Send(); // 发送拒绝原因
        ch->OnClose();
        delete ch;
        return ;
    }

    int try_count = 0;
    _client_port = _port + 1;   // 每次从最小端口开始尝试 应该对性能不会有很大的影响
    do{
        if(try_count > CLIENT_COUNT){
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
            GNET::Poll::register_poll(ch);  // 此时才会处理客户端
            GNET::Poll::register_poll(cl);  // 注册请求监听
            ch->set_client_listener(cl);    // 因为需要在与客户端断开的时候关闭监听 所以需要记录
            RunStatus::ClientInfo* ci = new RunStatus::ClientInfo;
            if (ci){
                ci->_active = false;
                ci->_server_port = _client_port;
                ci->_client_host = ch->get_host();
                ci->_client_port = ch->get_port();
                ci->_login_time = time(NULL);
                ci->_cur_sessions = 0;
                ci->_all_sessions = 0;
                ci->_data_size = 0;
                // 描述信息先不添加，保持异步处理
                RunStatus::add_client_info(ci);
            }else{OnClose();}
            break;
        }
    }while(1);
    ServerListener::dec_client_count(); // 客户端计数+1
};

int ServerListener::_client_count = 0;
int ServerListener::CLIENT_COUNT = 10;

void usege(){
    printf("使用方法: \n\tmain 端口 [客户端数量]\n\t默认是 main 7200 10\n");
}

int main(int argv, char* args[]){
    int port = 7200;
    int max_client = 10;
    if (argv == 2){
        port = atoi(args[1]);
    }else if(argv == 3){
        port = atoi(args[1]);
        max_client = atoi(args[2]);
    }else{
        usege();
    }
	GNET::Poll::init();
    ServerListener* sl = new ServerListener("0.0.0.0", port, max_client);
    if(sl->IsError()){
        printf("[Error]: 创建监听失败\n");
        delete sl;
        return -1;
    }
    
    // assert(false);
    GNET::Poll::loop_poll();
	getchar();
}
