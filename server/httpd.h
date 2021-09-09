#ifndef __HTTPD_H
#define __HTTPD_H

// 这个文件作为一个http服务器的简单实现，只是对http请求的简单响应

#include <rpoll.h>
#include <httpheader.h>

#include <vector>

class HttpRequestHandle : public GNET::BaseNet{
private:
    enum{
        RECV_MAX_BUFFER = 4096
    };
    char* _buffer;
public:
    HttpRequestHandle(GNET::BaseNet & bn) : 
        GNET::BaseNet(bn), _buffer(NULL){
            GNET::Poll::register_poll(this);    // 既然创建好对象了 那就表示连接顺利建立了
        }
    ~HttpRequestHandle(){
        if (_buffer){
            free(_buffer);
        }
    }

    void OnRecv(){
        if (!_buffer){
            _buffer = (char*)malloc(RECV_MAX_BUFFER);
            memset(_buffer, 0, RECV_MAX_BUFFER);
        }
        int ret = Recv(_buffer, RECV_MAX_BUFFER);
        if (ret <= 0){
            OnClose();
            delete this;
        }
        printf ("[Info]: 从浏览器接收到的信息: \n %50s\n", _buffer);
        Send("Content-Type: text/html\r\n\r\nHello~", strlen("Content-Type: text/html\r\n\r\nHello~"));
        OnClose();
    }
};

class Httpd : public GNET::Passive{
private:
    int _request_count;
public:
    Httpd(std::string host, int port) : GNET::Passive(host, port), _request_count(0){
        GNET::Poll::register_poll(this);
        printf("[Info]: 监听http %d\n", port);
    }

    void OnRecv(){  // 接收到来自于浏览器的请求 
        GNET::BaseNet* bn = Accept();
        if(!bn){
            printf("[Error]: 与浏览器建立连接失败\n");
            return ;
        }
        HttpRequestHandle *rh = new HttpRequestHandle(*bn);
        delete bn;
        _request_count ++;
    }

    void OnClose(){
        GNET::BaseNet::OnClose();
    }
}; 


#endif