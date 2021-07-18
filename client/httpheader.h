#ifndef __HTTPHEADER_H
#define __HTTPHEADER_H

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

/*
	解析http或https代理请求头
*/

class HttpHeader {
private:
	string _http_str;		// 完整的请求 (post)是会在body部分带数据的
	string _header_str;		// 不包含body
	string _path;			// 请求的路径 如果是GET则也带有参数
	vector<string> _lines;	// 以行分割
	map<string, string> _kv;// 键值对方式保存的请求参数
	string _method;			// 请求方法
	bool _is_https;			// 是否是https

	string error_str;		// 错误字符串

	int is_in(string& str, const char* _str) {
		int len;
		if ((len = str.find(_str)) < str.length()) {
			return len;
		}
		return -1;
	}
public:
	HttpHeader(string http_str) {
		_is_https = false;
		error_str = "";

		int header_end = is_in(http_str, "\r\n\r\n");
		if (header_end == -1) {
			error_str = "找不到请求头的尾部";
			return; 
		}
		_http_str = http_str;
		_header_str = http_str.substr(0, header_end);	// 获取请求头
		string tmp = _header_str;
		while (1) {
			int line_end = tmp.find("\r\n");
			if (line_end == -1) {
				_lines.push_back(tmp);
				tmp.clear();
				break;
			}
			string line = tmp.substr(0,line_end);
			_lines.push_back(line);
			tmp.erase(tmp.begin(), tmp.begin() + line_end + 2);
		}
		dump_lines();
		
		if (_lines.size() < 1) { return ; }

		string request_line = _lines[0];
		if (request_line.find("GET") == 0) {
			_method = "GET";
		}
		else if (request_line.find("GET") == 0) {
			_method = "POST";
		}
		else if (request_line.find("HEAD") == 0) {
			_method = "HEAD";
		}
		else if (request_line.find("CONNECT") == 0) {
			_method = "CONNECT";
			_is_https = true;
		}
		else {
			return;	// 不能处理的协议
		}

		for (int i = 1; i < _lines.size(); i++) {
			string& line = _lines[i];
			int p1 = is_in(line, ":");
			if (p1 == -1) {
				printf("[Error]: 解析请求行出错: %s\n", line.c_str());
				return;
			}
			string k = line.substr(0, p1);
			string v = line.substr(p1+1);
			if (v[0] == ' ') {
				v.erase(0,1);
			}
			_kv[k] = v;
		}
		dump_kv();
		// 获取_path

	}

	void dump_lines() {
		for (auto line : _lines) {
			cout << "item: " << line << endl;
		}
	}

	void dump_kv() {
		map<string, string>::iterator iter = _kv.begin();
		for (; iter != _kv.end(); iter++) {
			cout <<iter->first << "-" << iter->second << endl;
		}
	}
};

#endif