#ifndef __HTTPD_H
#define __HTTPD_H

// 这个文件作为一个http服务器的简单实现，只是对http请求的简单响应

#include <rpoll.h>
#include <httpheader.h>

#include <vector>
#include <map>

using namespace std;

char test_html[] = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>Hello~</h1>";

class ParamParse {
/*
    url中的参数将会传递到这里来，不过暂时没有完成这个类的必要
*/
private:
    map<string, string> _kv;

};

class ActionTask {
/*
    所有注册的Action都需要注册这里，以便实现回调
*/
public:
    virtual void OnRquest(ParamParse & pp){
        assert(false);
    }
};

class ActionManage {
/*
    每个请求的路径被认为是一个Action，每个Action会对应一个回调函数，将会在这里管理其映射关系
*/
private:
    map<string, ActionTask*> _kv;
public:
    ActionManage(){};

    void try_call(string path){
        map<string, ActionTask*>::iterator it = _kv.find(path);
        if (it != _kv.end()){
            // it->second->OnRquest();
        }
    }

    void register_action(string key, ActionTask* at){
        map<string, ActionTask*>::iterator it = _kv.find(key);
        if (it == _kv.end()){
            delete it->second;  // 如果已经注册过了，则会删除旧任务
        }
        _kv[key] = at;
    }
};

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
            memset(_buffer, 0, RECV_MAX_BUFFER);    // linux 不会把申请的内存置零，如果不手动清零的话，printf输出长度会错误
        }
        int ret = Recv(_buffer, RECV_MAX_BUFFER);
        if (ret <= 0){
            OnClose();
            delete this;
        }
        HttpHeader *hh = new HttpHeader(string(_buffer));
        // printf ("[Info]: 从浏览器接收到的信息: \n %s\n", _buffer);
        
        cout << "[Info]: 请求Path " << hh->get_path() << endl;
        Send(test_html, strlen(test_html));
        OnClose();
        delete hh;
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