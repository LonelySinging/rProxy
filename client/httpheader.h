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

	string error_str;		// �����ַ���

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
			error_str = "�Ҳ�������ͷ��β��";
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
		dump_kv();
		// ��ȡ_path

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