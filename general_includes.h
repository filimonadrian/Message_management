#ifndef GENERAL_INCLUDES_H
#define GENERAL_INCLUDES_H

#include <iostream>
#include <unordered_map>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <bits/stdc++.h> 

using namespace std;

#define BUFLEN 1600
#define TOPIC_LEN 50
#define MAX_CLIENTS 128

#define ONLINE 1
#define OFFLINE 0

#define SUBSCRIBE 1
#define UNSUBSCRIBE 0
#define ID_ACKNOWLEDGE 2
#define SF_ON 1
#define SF_OFF 0

#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3

typedef struct tcp_msg {
    char udp_ip[16];
    uint16_t udp_port;
    char topic[TOPIC_LEN];
    /*INT SHORT_REAL FLOAT STRING */
    uint8_t type;
    char payload[1500];
} tcp_msg;

typedef struct user_msg {
    /*subscribe or unsubscribe */
    uint8_t type;
    /*sf_on or sf_off */
    uint8_t sf;
    /*topic name */
    char topic[TOPIC_LEN];

}user_msg;

#endif