#include "packet.h"

#include <iostream>

/*
char* Packet::Pack(int sid, int data_len, char* data){
    if (!data || data_len < 0) { return NULL; }

    packet_data* pd = (packet_data*)malloc(HEADER_SIZE + data_len);
    if (!pd){return NULL;}

    pd->sid = sid;
    memcpy(pd->data, data, data_len);
    return (char*)pd;
}

// 解包的时候需要数据长度，真是滑稽
Packet::packet_data* Packet::UnPack(char* data, int data_len){
    if (!data){return NULL;}

    packet_data* pd = (packet_data*)data;
    unsigned short int sid = pd->sid;
    printf("[Debug]: packet(sid=%d)\n", sid);
    pd = (packet_data*)malloc(HEADER_SIZE + data_len);
    if (!pd){return NULL;}

    memcpy(pd, data, HEADER_SIZE + data_len);
    return pd;
}
*/
