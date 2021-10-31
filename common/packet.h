#ifndef __PACKET_H
#define __PACKET_H

#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "types.h"
#include "log.h"

class Packet{
public:
    enum{
        HEADER_SIZE = 2,                        // sid
        PACKET_SIZE = 4096,                     // 最大包大小
        DATA_SIZE   = PACKET_SIZE - HEADER_SIZE // 数据内容大小
    };

    typedef struct {
        us16 sid;
        char data[];
    }packet_data;

    Packet(int sid, int data_len, char* data) : _pd(NULL), _data_len(0){  // 打包
        if (!data){return ;}
        _pd = (packet_data*)malloc(data_len + HEADER_SIZE);
        if (!_pd) { return; }
        _pd->sid = sid;
        _data_len = data_len;
        memcpy(_pd->data, data, data_len);
    }

    Packet(char* data, int len) : _pd(NULL), _data_len(0) {     // 解包
        if (!data){return ;}
        // unsigned short int sid = ((packet_data*)data)->sid;
        _data_len = len - HEADER_SIZE;
        _pd = (packet_data*)malloc(len);
        if (!_pd) { return; }
        memcpy(_pd, data, len);
    }

    ~Packet(){if(_pd)free(_pd);};

    packet_data* get_pd(){if(!_pd){return NULL;}return _pd;};
    char* get_p(){return (char*)get_pd();};
    us16 get_sid(){if(!_pd){return 0;}return _pd->sid;};
    us16 get_data_len(){if(!_pd){return 0;}return _data_len;};
    char* get_data(){if(!_pd){return NULL;}return _pd->data;}
    size_t get_packet_len(){if(!_pd){return 0;}return (_data_len + HEADER_SIZE);};

    void dump(int len = 15){
        int pk_len = get_packet_len();
        int dump_len = (len<pk_len)?len: pk_len;
        LOG_D("dump(sid=%d, dump_len=%d): ", get_sid(), dump_len);
        for (int i=0;i<dump_len;i++){
            if (!(i % 15)){
                printf("\n");
            }
            printf("%02X ", ((char*)_pd)[i]);
        }
        printf("\n");
    }

    // 经典的打包解包方法
    // static char* Pack(int sid, int data_len, char* data);      // 打包
    // static packet_data* UnPack(char* data);                    // 解包

private:
    packet_data* _pd;
    size_t _data_len;
};

#endif
