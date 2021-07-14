#ifndef __RPOLL_H
#define __RPOLL_H

#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <map>

namespace GNET{
    using std::string;
    using std::cout;
    using std::endl;

    enum{
        MAX_CONNECT = 1024,
        LISTEN_COUNT = 50
    };

	class BaseNet{
	protected:
        string _host;
        int _port;
        int _sock_fd;
        unsigned int _flag;
        struct sockaddr_in _addr;
    public:
        enum {
            NET_ERROR = 0x0001     // 连接失败
        };
        int& get_sock(){return _sock_fd;};
        void set_sock(int sock){_sock_fd = sock;};
        string& get_host(){return _host;};
        void set_host(string host){_host = host;};
        int& get_port(){return _port;};
        void set_port(int port){_port = port;};
        struct sockaddr_in& get_sockaddr_in(){return _addr;};
        void set_sockaddr_in(struct sockaddr_in& addr){memcpy(&_addr, &addr, sizeof(addr));};

        BaseNet(){_flag = 0;};
        BaseNet(string host, int port):_host(host),_port(port){_flag = 0;};
        ~BaseNet(){};
        
        virtual void OnRecv(){
            printf("[Debug]: BaseNet的OnRecv()\n");
        };

        void OnClose(){
            // poll::deregister_poll(this);
            close(_sock_fd);
        };

        // 会接收一个连接，并且返回 BaseNet 对象指针
        BaseNet* Accept(){
            BaseNet* bn = new BaseNet();
            socklen_t lt = sizeof(_addr);
            int sock;
            struct sockaddr_in& addr = bn->get_sockaddr_in();
            if((sock = accept(_sock_fd, (struct sockaddr*)&(addr), &lt)) == -1){
                delete bn;
                return NULL;
            }
            bn->set_sock(sock);
            bn->set_port(ntohs(addr.sin_port));
            bn->set_host(inet_ntoa(addr.sin_addr));
            return bn;
        }

        virtual size_t Recv(char* data, size_t len){
            return recv(_sock_fd, data, len, 0);
        };

        virtual size_t Send(char* data, size_t len){
            return send(_sock_fd, data, len, 0);
        }
        unsigned int get_flag(){return _flag;};
        void SetError(){_flag |= NET_ERROR;}
        bool IsError(){return _flag & NET_ERROR;};
	};

    class Passive : public BaseNet{
    public:
        Passive(string host, int port):BaseNet(host,port){
            _sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (_sock_fd < 0){
                perror("[Error]: 创建服务套接字失败");
                SetError();
                return ;
            }
            memset(&_addr, 0, sizeof(_addr));
            if (inet_aton(_host.c_str(), &(_addr.sin_addr)) <= 0){
                perror("[Error]: 主机地址有错误");
                SetError();
                return ;
            }
            _addr.sin_family = AF_INET;
            _addr.sin_port = htons(_port);
            if(bind(_sock_fd, (sockaddr*)&_addr, sizeof(_addr)) < 0){
                perror("[Error]: 绑定错误");
                SetError();
                return ;
            }
            listen(_sock_fd, LISTEN_COUNT); // 需要错误处理
            printf("[Debug]: 开始监听, %s:%d\n", _host.c_str(), _port);
        };
        virtual void OnRecv(){
            printf("[Debug]: Passivs的虚方法\n");
        };
    };

    class Active : public BaseNet{
    public:
        Active(string host, int port): BaseNet(host, port){
            _sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if(_sock_fd < 0){
                perror("[Error]: 创建连接套接字失败");
                SetError();
                return ;
            }
            memset(&_addr, 0, sizeof(_addr));
            _addr.sin_family = AF_INET;
            _addr.sin_port = htons(_port);
            if(inet_pton(AF_INET, _host.c_str(), &_addr.sin_addr) <= 0){
                perror("[Error]: 主机地址有错误");
                SetError();
                return ;
            }
            if (connect(_sock_fd, (struct sockaddr*)&_addr, sizeof(_addr)) < 0){
                perror("[Error]: 连接错误");
                SetError();
                return ;
            }
            printf("[Info]: 连接成功: %s:%d", _host.c_str(), _port);
        }
    };

    class Poll{
    private:
        static struct epoll_event _ev, _events[MAX_CONNECT];
        static int _eph;
        static bool _running;
    public:
        Poll(){
            printf("[Debug]: 创建句柄\n");
            _eph = epoll_create(MAX_CONNECT);
            if(_eph < 0){perror("[Error]: 创建epoll错误");}
        };
        static int register_poll(BaseNet* bn){
            printf("[Debug]: 注册poll _sock_fd: %d, bn: %p\n", bn->get_sock(), bn);
            // _ev.events = EPOLLIN | EPOLLET;
            _ev.events = EPOLLIN;
            _ev.data.ptr = bn;
            epoll_ctl(_eph, EPOLL_CTL_ADD, bn->get_sock(), &_ev);
        }
        static void deregister_poll(BaseNet* bn){
            printf("[Debug]: 取消注册poll: %d\n", bn->get_sock());
            // 确实有必要吗
            epoll_ctl(_eph, EPOLL_CTL_DEL, bn->get_sock(), NULL);
        }
        static void loop_poll(){
            printf("[Debug]: 进入poll循环\n");
            _running = true;
            for(;_running;){
                int n = epoll_wait(_eph, _events, MAX_CONNECT, 500);
                // if(n != 0){printf("[Debug]: n=%d\n", n);};
                for(int i=0;i<n;i++){
                    // printf("[Debug]: loop_poll: %p\n",_events[i].data.ptr);
                    ((BaseNet* )_events[i].data.ptr)->OnRecv();  // 由多态选择具体的操作逻辑
                    // 这个确实是多态，没有问题。但是为什么会第一次失败，就不知道了
                }
            }
        }

        static void stop_poll(){
            _running = false;
        }
        
    };
};

#endif
