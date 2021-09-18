#ifdef __linux
#include <rthread.h>
#include <rpoll.h>
#include <unistd.h> 
#else
#include "thread/rthread.h"
#include "poll/rpoll.h"
#endif // _LINUX

#include "packet.h"

#include <types.h>
                   

#include <iostream>
#include <map>
#include <time.h>

#include "httpd.h"

using namespace std;

// 请求端的处理逻辑
class ClientHandle;
class RequestHandle : public GNET::BaseNet{
private:
    ClientHandle* _client_bn;
    int _sid;   // 每个请求对应一个sid
public:
    RequestHandle(GNET::BaseNet& bn, ClientHandle* client_bn, int sid) 
    : GNET::BaseNet(bn), _client_bn(client_bn), _sid(sid){
        printf("[Info]: 接收到了 %s:%d [%d] 的连接\n", _host.c_str(), _port, _sid);
    };    // 这里应该会触发 BaseNet 的拷贝构造

    void OnRecv();  // 来自请求端的数据
    void OnClose();
};


// 每个客户端对应一个实例
class ClientListener : public GNET::Passive{
public:

private:
    int _session;   // 生成 sid的依据
    int _client_fd;
    ClientHandle* _client_bn;    // 客户端的套接字

    int get_sid(){
        if (_session > MAX_SID){
            _session = MIX_SID;
        }
        return _session++;
    };

public:
    ClientListener(string host, int port, ClientHandle* bn) : Passive(host, port), _client_bn(bn){
        if(IsError()){
            printf("[Error]: 代理端口监听失败\n");
            return ;
        }
        _session = MIX_SID;
        _client_fd = 0;
    }
    void OnRecv();
};

// 客户端 Handle
class ClientHandle : public GNET::BaseNet{
private:
    friend RequestHandle;

    // 记录所有与之相关的请求端
    std::map<int, GNET::BaseNet*> _sessions;    // sid, client_bn
    ClientListener* _cl;        // 监听端口套接字，用于与客户端断开时关闭端口
    char* _buff;                // 接收缓存
    bool _active = false;       // 是否登录 比RunStatus里面的接口判断要快

public:
    ClientHandle(GNET::BaseNet& bn) 
    : GNET::BaseNet(bn), _cl(NULL){ // 使用默认的拷贝构造函数 浅拷贝够用了
        printf("[Info]: 与客户端 %s:%d 建立联系\n", _host.c_str(), _port);
        _buff = (char*)malloc(Packet::PACKET_SIZE + 2);
    };
    ~ClientHandle(){
        if (_buff){
            free(_buff);
            _buff = NULL;
        }
    }

    void OnRecv();  // 来自客户端的数据

    void set_client_listener(ClientListener *cl){_cl = cl;}
    GNET::BaseNet* fetch_bn(int sid = -1){
        if (sid == -1){ // 取到第一个请求端，调试用
            if (_sessions.begin() != _sessions.end()){
                return _sessions.begin()->second;
            }
        }else{
            std::map<int, GNET::BaseNet*>::iterator iter = _sessions.find(sid);
            if(_sessions.end() != iter){
                return iter->second;
            }
        }
        return NULL;
    }

    void add_session(int sid, GNET::BaseNet* bn){
        std::map<int, GNET::BaseNet*>::iterator iter = _sessions.find(sid);
        if (iter != _sessions.end()){
            assert(false && "sid重复");
            iter->second->OnClose();    // 有6000个sid可以用，但是依旧重复了，，不应该的
            delete iter->second;
        }
        _sessions[sid] = bn;    // 重复也覆盖
    }

    void del_session(int sid){
        if(sid == -1){
            std::map<int, GNET::BaseNet*>::iterator iter = _sessions.begin();
            for (;iter != _sessions.end();iter++){
                iter->second->OnClose();
                iter->second->SetDelete();  // 设置删除 会在poll循环中删除
                // delete iter->second;
                printf("[Info]: 结束会话 sid: %d\n", iter->first);
            }
            _sessions.clear();
        }else{
            std::map<int, GNET::BaseNet*>::iterator iter = _sessions.find(sid);
            if (iter != _sessions.end()){
                iter->second->OnClose();
                iter->second->SetDelete();  // 设置删除 会在poll循环中删除
                // delete iter->second;
                printf("[Info]: 结束会话 sid: %d\n", iter->first);
                _sessions.erase(iter);
            }
        }
    }

    void dump_sessions(){
        for (auto iter : _sessions){
            printf("[Debug]: dump(sid=%d, host=%s, port=%d, sock_fd=%d)\n",
                        iter.first, 
                        iter.second->get_host().c_str(),
                        iter.second->get_port(),
                        iter.second->get_sock());
        }
    }

    void send_cmd(char* data, int len);     // 往客户端发送命令的接口
    void OnClose();
};

// 这个类将会记录所有的客户端信息，和统计信息
class RunStatus{
public:
    typedef struct {                    // 客户端的基本属性 以端口作为索引
        bool _active;                   // 是否登录成功
        int _server_port;               // 服务器上面对应端口
        string _client_host;            // 客户端IP和端口
        int _client_port;
        time_t _login_time;             // 连接时间戳
        unsigned int _cur_sessions;     // 当前会话数量
        unsigned int _all_sessions;     // 历史会话数
        unsigned long _data_size;       // 经过的流量大小
        string _client_describe;        // 描述
    }ClientInfo;
    
    static ClientInfo* add_client_info(ClientInfo* ci){
        if (ci == NULL){return NULL;}
        map<int, ClientInfo*>::iterator it = _cis.find(ci->_server_port);
        if(it == _cis.end()){
            _cis[ci->_server_port] = ci;
            return ci;
        }else{
            return NULL;
        }
    }
    
    static bool del_client_info(int port){
        assert(port > 0 && port < 65536);
        map<int, ClientInfo*>::iterator it = _cis.find(port);
        if (it != _cis.end()){
            printf("[Info]: 与客户端断开%s:%d, 释放端口: %d\n", 
                _cis[port]->_client_host.c_str(),_cis[port]->_client_port, port);
            delete _cis[port];
            _cis.erase(it);
        }
        return true;
    }

    static int get_client_count(){
        return _cis.size();
    }

    static ClientInfo* get_client_info(int port){
        assert(port > 0 && port < 65536);
        map<int, ClientInfo*>::iterator it = _cis.find(port);
        if (it != _cis.end()){
            return _cis[port];
        }else{
            return NULL;
        }
    }

    static bool auth_login(char* pass){
        if (!pass){return false;}
        if(!strncmp(_passwd, pass, CMD::cmd_login::PASSWD_LEN)){
            return true;
        }
        return false;
    }

    static string to_html(){
        string status_html = "<meta charset=\"utf-8\">";    // 代码是utf-8的，所以常量字符串都是
        status_html += "当前连接数: "; status_html += to_string(get_client_count());
        status_html += "<hr>";
        status_html += "<table border=1px><th>监听端口</th><th>客户端IP端口</th><th>登录</th>";
        status_html += "<th>连接时间</th><th>当前会话数</th><th>历史会话数</th>";
        status_html += "<th>经过的流量大小</th><th>描述</th>";
        map<int, ClientInfo*>::iterator it = _cis.begin();
        for (;it != _cis.end();it++){
            string trs = "<tr>";
            trs += "<td>"; trs += to_string(it->second->_server_port);
            trs += "</td><td>"; trs += it->second->_client_host; trs += ":"; trs += to_string(it->second->_client_port);
            trs += "</td><td>"; trs += ((it->second->_active)?("<font color=#0F0>登录成功</font>"):("<font color=#F00>登录失败</font>"));
            trs += "</td><td>"; trs += to_string(it->second->_login_time);
            trs += "</td><td>"; trs += to_string(it->second->_cur_sessions);
            trs += "</td><td>"; trs += to_string(it->second->_all_sessions);
            trs += "</td><td>"; trs += to_string(it->second->_data_size);
            trs += "</td><td>"; trs += it->second->_client_describe;
            trs += "</td></tr>";
            status_html += trs;
        }
        status_html += "</table>";
        return status_html;
    }
    
private:
    static map<int, ClientInfo*> _cis;   // 保存了所有客户端信息 <端口, ci信息>
    static char _passwd[CMD::cmd_login::PASSWD_LEN];   // 密码 将来会放到conf中
};


class DumpRunStatusAction : public ActionTask{
public:
    virtual void OnRquest(HttpParam & pp, char* & data, int & data_len){
        string html = RunStatus::to_html();
        data_len = html.length();
        data = (char*)malloc(data_len);
        memcpy(data, html.data(), data_len);
    }
};

// 负责接收来自客户端的连接
class ServerListener : public GNET::Passive{
private:
    DumpRunStatusAction* _drsa;
    int _client_port;              // 记录下一个端口号
    static int _client_count;      // 活跃的客户端数
    static int CLIENT_COUNT;       // 客户端的最大数量
    int _port;
public:
    enum{
        // CLIENT_COUNT = 10,     // 客户端数量数量
        MAX_TRY_NUM = 10        // 最大尝试绑定端口数
    };
    ServerListener(string host, int port, int max_client=10):Passive(host, port), _port(port){
        if(IsError()){  // 连接出错就直接退出
            return ;
        }
        CLIENT_COUNT = max_client;
        _client_port = _port;    // 客户端监听从服务端口+1开始
        _client_count = 0;
        GNET::Poll::register_poll(this);
        _drsa = new DumpRunStatusAction();
        HttpRequestHandle::register_action("/dumpState", _drsa);
    };

    ~ServerListener(){
        delete _drsa;
    }

    void OnRecv();

    int get_client_count(){return _client_count;}
    static void inc_client_count(){
        _client_count--;
        if(_client_count < 0){
            printf("[Warning]: 客户端计数小于0\n");
            _client_count = 0;
        }
    };
    static void dec_client_count(){
        _client_count++;
        if(_client_count > CLIENT_COUNT){
            printf("[Warning]: 超出客户端上限 Real/Max: %d/%d\n", _client_count, CLIENT_COUNT);
        }
    }
};



