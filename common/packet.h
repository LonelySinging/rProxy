#ifndef __PACKET_H
#define __PACKET_H

#include <string.h>
#include <stdlib.h>
#include <iostream>

class Packet{
public:
    enum{
        HEADER_SIZE = 2,                        // sid
        PACKET_SIZE = 4096,                     // 最大包大小
        DATA_SIZE   = PACKET_SIZE - HEADER_SIZE // 数据内容大小
    };

    typedef struct {
        unsigned short int sid;
        char data[];
    }packet_data;

    Packet(int sid, int data_len, char* data) : _pd(NULL){  // 打包
        if (!data){return ;}
        _pd = (packet_data*)malloc(HEADER_SIZE + data_len);
        _pd->sid = sid;
        _data_len = data_len;
        memcpy(_pd->data, data, data_len);
    }

    Packet(char* data, int len) : _pd(NULL){     // 解包
        if (!data){return ;}
        // unsigned short int sid = ((packet_data*)data)->sid;
        _data_len = len;
        _pd = (packet_data*)malloc(HEADER_SIZE + _data_len);
        memcpy(_pd, data, HEADER_SIZE + _data_len);
    }

    ~Packet(){free(_pd);};

    packet_data* get_pd(){if(!_pd){return NULL;}return _pd;};
    char* get_p(){return (char*)get_pd();};
    unsigned short int get_sid(){if(!_pd){return 0;}return _pd->sid;};
    unsigned short int get_data_len(){if(!_pd){return 0;}return _data_len;};
    char* get_data(){if(!_pd){return NULL;}return _pd->data;}
    size_t get_packet_len(){if(!_pd){return 0;}return (_data_len + HEADER_SIZE);};

    void dump(int len = 15){
        int dump_len = std::min(len, (int)get_packet_len());
        printf("[Debug]: dump(sid=%d, dump_len=%d): ", get_sid(), dump_len);
        for (int i=0;i<dump_len;i++){
            if (!(i % 15)){
                printf("\n");
            }
            printf("%x ", ((char*)_pd)[i]);
        }
        printf("\n");
    }

    // 经典的打包解包方法
    static char* Pack(int sid, int data_len, char* data);      // 打包
    static packet_data* UnPack(char* data);                    // 解包

private:
    packet_data* _pd;
    size_t _data_len;
};

#endif
