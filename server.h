#ifndef SERVER_H
#define SERVER_H

//#include "server_helper.h"
#include "general_includes.h"


/*received message from udp clients */
typedef struct __attribute__((packed)) udp_msg {
    char topic[50];
    uint8_t type;
    char payload[1500];
} udp_msg;

typedef struct tcp_client {
    char client_id[11];
    uint32_t sockfd;
    /* ONLINE or OFFLINE */
    uint8_t status;

} tcp_client;

/*last message send */
typedef struct news_status {
    uint32_t last_msg;
    uint32_t sf;
    string topic;
}news_status;


#endif