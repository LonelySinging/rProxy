#ifndef __RPOLL_H
#define __RPOLL_H

#ifdef __linux
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#endif

#include <string.h>


#include <iostream>
#include <map>

namespace GNET {
    using std::string;
    using std::cout;
    using std::endl;
    using std::map;

    enum {
        MAX_CONNECT = 1024,
        LISTEN_COUNT = 50
    };

    class BaseNet {
    protected:
        string _host;
        int _port;
        int _sock_fd;
        unsigned int _flag;
        struct sockaddr_in _addr;
        char* _buffer; // 接收缓冲区    // 记得初始化
        int _packet_size;  // 实际包大小   记得初始化
        int _packet_pos;    // 已经接收的实际大小
    public:
        enum {
            NET_ERROR = 0x0001     // 连接失败
        };
        
        // 基础包结构，只有数据大小头
        typedef struct {
            unsigned short int data_len;
            char data[];
        }BasePacket;

        int& get_sock() { return _sock_fd; };
        void set_sock(int sock) { _sock_fd = sock; };
        string& get_host() { return _host; };
        void set_host(string host) { _host = host; };
        int& get_port() { return _port; };
        void set_port(int port) { _port = port; };
        struct sockaddr_in& get_sockaddr_in() { return _addr; };
        void set_sockaddr_in(struct sockaddr_in& addr) { memcpy(&_addr, &addr, sizeof(addr)); };

        BaseNet() : _flag(0), _buffer(NULL), _packet_size(0), _packet_pos(0) {};
        BaseNet(string host, int port) :_host(host), _port(port), _flag(0),
                                        _buffer(NULL),
                                        _packet_size(0),
                                        _packet_pos(0){};
        ~BaseNet() {};

        virtual void OnRecv() {
            printf("[Debug]: BaseNet的OnRecv()\n");
        };

        void OnClose() {
            // poll::deregister_poll(this);
            if (_buffer){
                free(_buffer);  
                // 当套接字被关闭的时候，缓冲区肯定没用了
                // 但是因为可能copy，所以不能在析构函数中free 
                // 否则会导致delete副本之后所有的对象_buffer失效
            }
#ifdef __linux
            close(_sock_fd);
#else
            closesocket(_sock_fd);
#endif
        };

        // 会接收一个连接，并且返回 BaseNet 对象指针
        BaseNet* Accept() {
            BaseNet* bn = new BaseNet();
#ifdef __linux
            socklen_t lt = sizeof(_addr);
#else
            int lt = sizeof(_addr);
#endif
            int sock;
            struct sockaddr_in& addr = bn->get_sockaddr_in();
            if ((sock = accept(_sock_fd, (struct sockaddr*)&(addr), &lt)) == -1) {
                delete bn;
                return NULL;
            }
            bn->set_sock(sock);
            bn->set_port(ntohs(addr.sin_port));
            bn->set_host(inet_ntoa(addr.sin_addr));
            return bn;
        }

        int Recv(char* data, size_t len) {
            return recv(_sock_fd, data, len, 0);
        };

        int Send(char* data, size_t len) {
            return send(_sock_fd, data, len, 0);
        }

        // 发送包，返回发送出去的数据，不包含长度数据的长度
        int SendPacket(char* data, size_t len) {
            BasePacket* tmp = (BasePacket*) malloc(len + sizeof(unsigned short int));
            tmp->data_len = len;
            memcpy(tmp->data, data, len);
            int ret = Send((char*)tmp, len + sizeof(unsigned short int));
            free(tmp);
            return ret;
        };

        // 返回 -1 是还没有收到完整包，应该忽略，返回0表示断开连接
        // 很极端的情况？一次接收甚至没有接收到完整的基本包头。。。
        int RecvPacket(char* data, size_t expected_len) {
            char* tmp = (char*)malloc(expected_len);
            if (!_buffer){
                int ret = Recv(tmp, expected_len);
                if ((ret == 0) || (ret == -1)){return 0;}    // 断开连接
                _packet_size = ((BasePacket*)tmp)->data_len;
                if (ret == (_packet_size + sizeof(unsigned short int))){    // 一次就接收了完整包
                    memcpy(data, ((BasePacket*)tmp)->data, _packet_size);
                    free(tmp);
                    return _packet_size;
                }else{  // 接收到了半个包
                    _buffer = (char*)malloc(expected_len);
                    int real_recv = ret - sizeof(unsigned short int);
                    memcpy(_buffer + _packet_pos, ((BasePacket*)tmp)->data, real_recv);

                    _packet_pos += real_recv;  // 记录已经接收的大小
                    free(tmp);
                    return -1;
                }
            }else{
                int ret = Recv(tmp, (_packet_size - _packet_pos));    // 尝试接收包的剩余部分
                if ((ret == 0) || (ret == -1)){return 0;}    // 断开连接
                memcpy(_buffer + _packet_pos, tmp, ret);
                _packet_pos += ret;
                if (_packet_size == _packet_pos){   // 接收完毕
                    memcpy(data, _buffer, _packet_pos);
                    free(tmp);
                    free(_buffer);
                    _buffer = NULL;
                    _packet_pos = 0;
                    // _packet_size = 0;
                    return _packet_size;
                }else{  // 还没有接收完
                    free(tmp);
                    return -1;
                }
            }
        };

        unsigned int get_flag() { return _flag; };
        void SetError() { _flag |= NET_ERROR; }
        bool IsError() { return _flag & NET_ERROR; };
    };

    class Passive : public BaseNet {
    public:
        Passive(string host, int port) :BaseNet(host, port) {
            _sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (_sock_fd < 0) {
                perror("[Error]: 创建服务套接字失败");
                SetError();
                return;
            }
            memset(&_addr, 0, sizeof(_addr));
            if (inet_pton(AF_INET, _host.c_str(), &(_addr.sin_addr)) <= 0) {
                perror("[Error]: 主机地址有错误");
                SetError();
                return;
            }
            _addr.sin_family = AF_INET;
            _addr.sin_port = htons(_port);
            if (bind(_sock_fd, (sockaddr*)&_addr, sizeof(_addr)) < 0) {
                perror("[Error]: 绑定错误");
                SetError();
                return;
            }
            listen(_sock_fd, LISTEN_COUNT); // 需要错误处理
            printf("[Debug]: 开始监听, %s:%d\n", _host.c_str(), _port);
        };
        virtual void OnRecv() {
            printf("[Debug]: Passivs的虚方法\n");
        };
    };

    class Active : public BaseNet {
    public:
        Active(string host, int port) : BaseNet(host, port) {
            _sock_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (_sock_fd < 0) {
                perror("[Error]: 创建连接套接字失败");
                SetError();
                return;
            }
            memset(&_addr, 0, sizeof(_addr));
            _addr.sin_family = AF_INET;
            _addr.sin_port = htons(_port);
            if (inet_pton(AF_INET, _host.c_str(), &_addr.sin_addr) <= 0) {
                perror("[Error]: 主机地址有错误");
                SetError();
                return;
            }
            if (connect(_sock_fd, (struct sockaddr*)&_addr, sizeof(_addr)) < 0) {
                perror("[Error]: 连接错误");
                SetError();
                return;
            }
            printf("[Info]: 连接成功: %s:%d\n", _host.c_str(), _port);
        }
    };

    class Poll {
    public:
        enum {
            TIMEOUT = 500  // 超时时间 毫秒
        };
    private:
#ifdef __linux
        static struct epoll_event _ev, _events[MAX_CONNECT];
        static int _eph;
#else
        static timeval _select_timeout;
        static fd_set _read_fds;
        static map<int, BaseNet*> _read_fds_map;

#endif
        static bool _running;
    public:
        Poll() {
            printf("[Debug]: 创建句柄\n");
#ifdef __linux
            _eph = epoll_create(MAX_CONNECT);
            if (_eph < 0) { perror("[Error]: 创建epoll错误"); }
#else

            // FD_ZERO(&_read_fds);
#endif
        };

#ifdef __linux
        ;
#else
        static void init_select() {
            _select_timeout.tv_sec = 0;
            _select_timeout.tv_usec = TIMEOUT;
            FD_ZERO(&_read_fds);
        }
#endif

        static void register_poll(BaseNet* bn) {
            printf("[Debug]: 注册poll _sock_fd: %d, bn: %p\n", bn->get_sock(), bn);

#ifdef __linux
            // _ev.events = EPOLLIN | EPOLLET;
            _ev.events = EPOLLIN;
            _ev.data.ptr = bn;
            epoll_ctl(_eph, EPOLL_CTL_ADD, bn->get_sock(), &_ev);
#else
            FD_SET(bn->get_sock(),&_read_fds);
            _read_fds_map[bn->get_sock()] = bn;
#endif
        }
        static void deregister_poll(BaseNet* bn) {
            printf("[Debug]: 取消注册poll: %d\n", bn->get_sock());
            // 确实有必要吗
#ifdef __linux
            epoll_ctl(_eph, EPOLL_CTL_DEL, bn->get_sock(), NULL);
#else
            FD_CLR(bn->get_sock(), &_read_fds);     // 忘记取消注册的话，会导致select检查已经被关闭的套接字 导致出现 10038 错误
            _read_fds_map.erase(bn->get_sock());
#endif
        }
        static void loop_poll() {
            printf("[Debug]: 进入poll循环\n");
            _running = true;
            for (; _running;) {
#ifdef __linux
                int n = epoll_wait(_eph, _events, MAX_CONNECT, TIMEOUT);
                // if(n != 0){printf("[Debug]: n=%d\n", n);};
                for (int i = 0; i < n; i++) {
                    // printf("[Debug]: loop_poll: %p\n",_events[i].data.ptr);
                    ((BaseNet*)_events[i].data.ptr)->OnRecv();  // 由多态选择具体的操作逻辑
                    // 这个确实是多态，没有问题。但是为什么会第一次失败，就不知道了
                }
#else
                // printf("[Debug]: t1=%d, t2=%d\n", _select_timeout.tv_sec, _select_timeout.tv_usec);
                int n = select(0, &_read_fds, NULL, NULL, /*&_select_timeout*/NULL);
                // printf("[Debug]: 1: %d, 2: %d\n", _read_fds.fd_count, _read_fds.fd_array[0]);
                if (n == SOCKET_ERROR) {
                    printf("[Error]: Select 错误 WSAGetLastError: %d\n", WSAGetLastError());
                    stop_poll();
                    WSACleanup();
                    continue;
                };
                if (n == 0) { continue; };

                map<int, BaseNet*>::iterator iter = _read_fds_map.begin();
                for (; iter != _read_fds_map.end(); iter++) {
                    if (FD_ISSET(iter->first, &_read_fds)) {
                        iter->second->OnRecv();
                        iter = _read_fds_map.begin();   // 可能会取消注册导致结构变化 所以从头开始
                    }
                }
#endif
            }

        }

        static void stop_poll() {
            _running = false;
        }

    };
};

#endif
