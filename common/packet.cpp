#include "packet.h"

#include <iostream>

char* Packet::Pack(int sid, int data_len, char* data){
    if (!data || data_len < 0) { return NULL; }

    packet_data* pd = (packet_data*)malloc(HEADER_SIZE + data_len);
    if (!pd){return NULL;}

    pd->sid = sid;
    pd->data_len = data_len;

    memcpy(pd->data, data, data_len);

    return (char*)pd;
}

Packet::packet_data* Packet::UnPack(char* data){
    if (!data){return NULL;}

    packet_data* pd = (packet_data*)data;
    unsigned short int sid = pd->sid;
    unsigned short int data_len = pd->data_len;
    printf("[Debug]: packet(sid=%d, data_len=%d)\n", sid, data_len);
    pd = (packet_data*)malloc(HEADER_SIZE + data_len);
    if (!pd){return NULL;}

    memcpy(pd, data, HEADER_SIZE + data_len);
    return pd;
}
