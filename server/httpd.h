#ifndef __HTTPD_H
#define __HTTPD_H

// 这个文件作为一个http服务器的简单实现，只是对http请求的简单响应

#include <rpoll.h>
#include <httpheader.h>

#include <vector>
#include <map>

using namespace std;

class HttpParam {
/*
    url中的参数将会使用这种方式保存
*/
private:
    map<string, string> _kv;
    string _param_str;

public:
    HttpParam(){}

    // 返回的是参数的数量，如果有重复的k，返回值是去重之前的数量
    int setup(string & param_str) {
        _param_str = param_str;
        int p0 = 0;     // &
        int p1 = 0;     // =
        int kv_count = 0;
        do {
            p1 =  HttpHeader::is_in(_param_str, "=", p0);
            if (p1 == -1) {break ;}
            string k = param_str.substr(p0, p1 - p0);
            p0 = HttpHeader::is_in(_param_str, "&");
            p1 ++;
            if (p1 >= _param_str.length()){return -1;}
            if (p0 == -1){
                string v = param_str.substr(p1, param_str.length() - p1);   // 最后一个键值对了，等号后面的都认为是v    ii=99&ee=9
                kv_count ++;
                _kv[k] = v;
                break ;
            }
            string v = param_str.substr(p1, p0 - p1);
            _kv[k] = v;
            kv_count ++;
            p0 ++;
            if (p0 >= _param_str.length()){return -1;}
        }while (p0 != -1);
        return kv_count;
    }

    bool has_key(string & k){
        map<string, string>::iterator it = _kv.find(k);
        if (it == _kv.end()){
            return false;
        }
        return true;
    }

    string get_value(string & k){
        map<string, string>::iterator it = _kv.find(k);
        if (it == _kv.end()){
            return "";
        }
        return it->second;
    }
};

class ActionTask {
/*
    所有注册的Action都需要注册这里，以便实现回调
*/
public:
    virtual void OnRquest(HttpParam & pp, char* & data, int & data_len){
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

    string get_key(string & path) {
        if (path == "") {return "";}
        int p1 = 0;
        if ((p1=HttpHeader::is_in(path, "?")) == -1){    // 没有找到参数
            return path;
        }
        return path.substr(0, p1);
    }

    bool get_param(string & path, HttpParam & hp) {
        if (path == "") {return false;}
        int p1 = 0;
        if ((p1=HttpHeader::is_in(path, "?")) == -1){    // 没有找到参数
            return false;
        }
        p1 ++;
        string param_str = path.substr(p1, path.length() - p1);
        hp.setup(param_str);
    }

    bool try_call(string path, char* & data, int & data_len){
        map<string, ActionTask*>::iterator it = _kv.find(get_key(path));
        if (it != _kv.end()){
            HttpParam hp;
            get_param(path, hp);
            it->second->OnRquest(hp, data, data_len);
            return true;
        }
        return false;
    }

    void register_action(string key, ActionTask* at){
        map<string, ActionTask*>::iterator it = _kv.find(key);
        if (it != _kv.end()){
            delete it->second;  // 如果已经注册过了，则会删除旧任务
        }
        _kv[key] = at;
    }

    void cancel_action(string key){
        map<string, ActionTask*>::iterator it = _kv.find(key);
        if (it != _kv.end()){
            delete it->second;  // 如果已经注册过了，则会删除旧任务
            _kv.erase(key);
        }
    }
};

class HttpRequestHandle : public GNET::BaseNet{
private:
    enum{
        RECV_MAX_BUFFER = 4096
    };
    static ActionManage _am;
    char* _buffer;
    string html_header;
public:
    HttpRequestHandle(GNET::BaseNet & bn) : 
        GNET::BaseNet(bn), _buffer(NULL){
            GNET::Poll::register_poll(this);    // 既然创建好对象了 那就表示连接顺利建立了
            html_header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        }
    ~HttpRequestHandle(){
        if (_buffer){
            free(_buffer);
        }
    }

    // 因为需要把数据把main.cpp那边传递过来，又不想两个模块之间有太多的耦合所以还是使用老办法
    static void register_action(string key, ActionTask* at){
        cout << "[Info]: 注册" << key << endl;
        _am.register_action(key, at);
    }

    string make_html(string body){
        return (html_header+body);
    }

    int send_html(string body){
        string send_data = html_header + body;
        return Send(send_data.c_str(), send_data.length());
    }

    int send_html_404(){
        string send_data = html_header + string("<h1>404</h1>");
        return Send(send_data.c_str(), send_data.length());
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

        char* data = NULL;
        int data_len = 0;
        if (_am.try_call(hh->get_path(), data, data_len)){
            send_html(string(data, data_len));
        }else{
            send_html_404();
        }

        if (data){
            free(data);
            data = NULL;
            data_len = 0;
        }
        
        cout << "[Info]: 请求Path " << hh->get_path() << endl;
        // Send(html_header, strlen(html_header));
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