#ifndef SERVER_H
#define SERVER_H

//#include "server_helper.h"
#include "general_includes.h"


/*received message from udp clients */
typedef struct udp_msg {
    char topic[51];
    uint8_t type;
    char payload[1501];
} udp_msg;

typedef struct tcp_client {
    char client_id[11];
    uint32_t sockfd;
    /* ONLINE or OFFLINE */
    uint8_t status;

} tcp_client;

/*last message send */
typedef struct news_status {
    // char user_id[11];
    uint32_t last_msg;
    uint8_t sf;
    // char topic[51];
    string topic;
}news_status;

typedef struct mesaj {
    int sockfd;
    int name;
} mesaj;

// map with <topic_name> <all_subscribers_for_this_topic>
// unordered_map<string, vector<tcp_client>> forum_tcp;
// unordered_map<string, vector<tcp_client>> forum_udp;

#endif