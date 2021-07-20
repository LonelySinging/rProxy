#ifndef __HTTPHEADER_H
#define __HTTPHEADER_H

#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;

/*
	����http��https��������ͷ
*/

class HttpHeader {
private:
	string _http_str;		// ���������� (post)�ǻ���body���ִ����ݵ�
	string _header_str;		// ������body
	string _path;			// �����·�� �����GET��Ҳ���в���
	vector<string> _lines;	// ���зָ�
	map<string, string> _kv;// ��ֵ�Է�ʽ������������
	string _method;			// ���󷽷�
	bool _is_https;			// �Ƿ���https
	string _host;			
	int _port;

	string error_str;		// �����ַ���

	int is_in(string& str, const char* _str, int start = 0) {
		int len;
		if ((len = str.find(_str, start)) < str.length()) {
			return len;
		}
		return -1;
	}
public:
	HttpHeader(string http_str) {
		_is_https = false;

		_host = "";
		_port = 0;

		int header_end = is_in(http_str, "\r\n\r\n");
		if (header_end == -1) {
			printf("[Warning]: �Ҳ�������ͷ��β��\n");
			return; 
		}
		_http_str = http_str;
		_header_str = http_str.substr(0, header_end);	// ��ȡ����ͷ
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
		// dump_lines();
		
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
			_method = "";
			return;	// ���ܴ����Э��
		}

		for (int i = 1; i < _lines.size(); i++) {
			string& line = _lines[i];
			int p1 = is_in(line, ":");
			if (p1 == -1) {
				printf("[Error]: ���������г���: %s\n", line.c_str());
				return;
			}
			string k = line.substr(0, p1);
			string v = line.substr(p1+1);
			if (v[0] == ' ') {
				v.erase(0,1);
			}
			_kv[k] = v;
		}
		// dump_kv();
		// ��ȡ_path

	}

	void dump_lines() {
		for (auto line : _lines) {
			cout << "item: " << line << endl;
		}
	}

	bool has_key(string key) {
		map<string, string>::iterator iter = _kv.find(key);
		if (iter == _kv.end()) {
			return false;
		}
		else {
			return true;
		}
	}

	string get_value(string key) {
		map<string, string>::iterator iter = _kv.find(key);
		if (iter == _kv.end()) {
			return "";
		}
		else {
			return iter->second;
		}
	}

	void dump_kv() {
		map<string, string>::iterator iter = _kv.begin();
		for (; iter != _kv.end(); iter++) {
			cout <<iter->first << "-" << iter->second << endl;
		}
	}

	string get_host() {
		if (_host != "") {
			return _host;
		}
		if (has_key("Host")) {	// connectЭ��Ҳ���ܻ��� Host �ֶ� ����ʹ��
			string host_ip = get_value("Host");
			int pos = is_in(host_ip, ":");
			if (pos == -1) {
				_host = host_ip;
				_port = (_is_https)?443:80;
			}
			else {
				_host = host_ip.substr(0, pos);
				_port = atoi(host_ip.substr(pos+1,host_ip.length()).c_str());
			}
		}
		else if(_is_https){	// û��Host�Ļ���ֻ��ͨ������CONNECT�õ���������
			string first_line = _lines[0];
			int pos = is_in(first_line, " ");
			if (pos == -1) {
				return "";
			}
			if (pos + 1 > first_line.length()) { return ""; }
			int p2 = is_in(first_line, "HTTP");
			if (p2 == -1) {
				return "";
			}
			string host_port = first_line.substr(pos+1, (p2-1)-(pos+1));
			int p3 = is_in(host_port, ":");
			if (p3 == -1) {
				_host = host_port;
				_port = 443;
			}
			else {
				_host = host_port.substr(0, p3);
				_port = atoi(host_port.substr(p3 + 1, host_port.length() - (p3 + 1)).c_str());
			}
			
		}
		return _host;
	}

	string rewrite_header() {
		if (_is_https) {
			return _http_str;
		}
		else {
			string str = "http://" + get_value("Host");
			string tmp = _http_str;
			int pos = tmp.find(str);
			if (pos > tmp.length()) {
				printf("[Warning]: �Ҳ���ͷ��������\n");
				return tmp;	// ���Է���
			}
			tmp.erase(pos, str.length());
			return tmp;
		}
	}

	int get_port() {
		if (_port == 0) {
			get_host();
		}
		return _port;
	}

	string get_method() {
		return _method;
	}
};

#endif