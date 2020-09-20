#include "server.h"
#include "server_helper.h"

int main(int argc, char *argv[]) {

    int tcp_sock, udp_sock, newsockfd, portno;
    char buffer[BUFLEN];
    struct sockaddr_in tcp_addr, udp_addr, from_station;
    socklen_t source_len;
    string topic, user_id;
    user_msg message;

    int i, ret;
    int received_data, send_test;
    // vector<tcp_client> tcp_users;

    /* <user_id> and <socket> to update file descriptor*/
    unordered_map<string, int> id_sock;

    /*<user_id> and <vector_news> with topic, 
    sf, and index of last send message*/
    /* used to check if user wants all informations about topic;
     keep last message sent */
    unordered_map<string, vector<news_status>> user_subscription;

    /*<topic_name> and vector of messages ready to send*/
    unordered_map<string, vector<tcp_msg>> queue_msg;

    /*all active connections for a topic */
    unordered_map<string, vector<int>> active_users;

    fd_set read_fds;
    fd_set tmp_fds;
    int fdmax; // max fd from read_fds

    if (argc < 2) {
        usage(argv[0]);
    }

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    /* create TCP socket */
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("Can't open TCP socket");
    }
    /* create UDP socket */
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("Can't open UDP socket");
    }

    portno = atoi(argv[1]);
    if (portno < 0) {
        perror("Can't use this port");
    }

    memset((char *)&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(portno);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    int enable = 1;

    memset((char *)&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(portno);
    udp_addr.sin_addr.s_addr = INADDR_ANY;

    // this socket is always free(for repeted tests)
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
        perror("setsocketopt");
        exit(1);
    }
    ret = bind(tcp_sock, (struct sockaddr *)&tcp_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        perror("TCP can't bind");
    }

    ret = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(udp_addr));
    if (ret < 0) {
        perror("UDP can't bind");
    }

    ret = listen(tcp_sock, MAX_CLIENTS);
    if (ret < 0) {
        perror("TCP can't listen");
    }

    /* add the socket that listen for tcp connections*/
    FD_SET(tcp_sock, &read_fds);
    /* add the socket for udp connections */
    FD_SET(udp_sock, &read_fds);
    /*add the file descriptor that read from stdin */
    FD_SET(0, &read_fds);

    if (tcp_sock > udp_sock) {
        fdmax = tcp_sock;
    } else {
        fdmax = udp_sock;
    }

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("Can't select new fd:");
        }

        if (FD_ISSET(0, &tmp_fds)) {
            if (close_server(read_fds, fdmax)) {
                break;
            } else {
                continue;
            }
        }

        for (i = 1; i <= fdmax; i++) {

            if (FD_ISSET(i, &tmp_fds)) {
                if (i == udp_sock) {

                    memset(buffer, 0, BUFLEN);
                    memset(&from_station, 0, sizeof(from_station));
                    source_len = sizeof(from_station);
                    received_data = recvfrom(udp_sock, buffer, sizeof(udp_msg),
                                             0, (struct sockaddr *)&from_station, &source_len);
                    if (received_data < 0) {
                        perror("Can't receive udp message");
                    }
                    /* unwrap the udp message */
                    udp_msg message;
                    tcp_msg ready_to_send;
                    memset(&message, 0, sizeof(message));
                    memset(&ready_to_send, 0, sizeof(ready_to_send));
                    memcpy(&message, buffer, sizeof(message));

                    ready_to_send.udp_port = ntohs(from_station.sin_port);
                    strcpy(ready_to_send.udp_ip, inet_ntoa(from_station.sin_addr));
                    memcpy(ready_to_send.topic, message.topic, sizeof(message.topic));
                    memcpy(ready_to_send.payload, message.payload, sizeof(message.payload));

                    ready_to_send.type = message.type;
                    string topic = message.topic;

                    /* if it's a new topic, create new entry in queue and in active_users */
                    if (queue_msg.find(topic) == queue_msg.end()) {
                        queue_msg.emplace(topic, vector<tcp_msg>());
                        /*push message to the msg_queue(for other users) */
                        queue_msg[topic].push_back(ready_to_send);
                        active_users.emplace(topic, vector<int>());

                    } else {
                        /* if topic exists, send message to active users and
                        push message to the msg_queue(for other users) */
                        queue_msg[topic].push_back(ready_to_send);
                        /* send to all active users for this topic */
                        memset(buffer, 0, BUFLEN);
                        memcpy(buffer, &ready_to_send, sizeof(tcp_msg));
                        for (auto element : active_users[topic]) {
                            send_test = send(element, buffer, BUFLEN, 0);
                            if (send_test < 0) {
                                perror("Can't send message to active user");
                            }
                        }
                    }

                    // print_tables(id_sock, user_subscription, queue_msg, active_users);

                } else if (i == tcp_sock) {

                    /* new connection request 
						server accepts the request */
                    source_len = sizeof(from_station);
                    memset(&from_station, 0, sizeof(from_station));
                    newsockfd = accept(tcp_sock, (struct sockaddr *)&from_station, &source_len);

                    if (newsockfd < 0) {
                        perror("Can't accept new clients!");
                    }

                    // disable Neagle's algorithm
                    setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));

                    /* Now i need the user id*/

                    /*clean buffer*/
                    memset(buffer, 0, BUFLEN);
                    received_data = recv(newsockfd, buffer, sizeof(buffer), 0);
                    if (received_data < 0) {
                        perror("Can't receive user id!");
                    }
                    /*unwrap the message */
                    /*The buffer contains just THE USER ID */
                    string user_id = buffer;

                    /*if user_id doesn't exist, create a new entry */
                    if (id_sock.find(user_id) == id_sock.end()) {
                        id_sock.insert({user_id, newsockfd});
                        user_subscription.emplace(user_id, vector<news_status>());

                        /*else the user_id is back online 
                        -->update sockfd, tick as active user, send messages */
                    } else {

                        // verify if user is online and wants to connect
                        /*if user is online */
                        if (id_sock[user_id] != OFFLINE) {
                            close(newsockfd);
                            break;
                        }

                        id_sock[user_id] = newsockfd;
                        /*send each message which has sf on */
                        /* for each topic of this new user */

                        for (auto element : user_subscription[user_id]) {
                            /*tick as active user for current topic */
                            (active_users[element.topic]).push_back(newsockfd);
                            if (element.sf == SF_ON) {
                                /* send last messages */
                                int len = queue_msg[element.topic].size();
                                for (int k = element.last_msg + 1; k < len; k++) {
                                    memset(buffer, 0, BUFLEN);
                                    memcpy(buffer, &queue_msg[element.topic][k], BUFLEN);
                                    send_test = send(newsockfd, buffer, BUFLEN, 0);
                                    if (send_test < 0) {
                                        perror("Can't send stored messages");
                                    }
                                }
                            }
                        }
                    }

                    /* add new fd to read_fds */
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    cout << "New client " << user_id << " connected from " << ntohs(from_station.sin_port);
                    cout << ":" << inet_ntoa(from_station.sin_addr) << endl;

                    // print_tables(id_sock, user_subscription, queue_msg, active_users);

                } else {
                    /* i've received data on an existing socket
						the server must interpret them */
                    memset(buffer, 0, BUFLEN);

                    received_data = recv(i, buffer, sizeof(buffer), 0);
                    if (received_data < 0) {
                        perror("Receive TCP messages problem");
                    } else if (received_data == 0) {
                        /* the connection has ended */
                        string user_id = get_id_for_socket(i, id_sock);

                        disconnect_user(id_sock, user_subscription, queue_msg, active_users, i);
                        /* socket = 0 ===> user is offline */
                        id_sock[get_id_for_socket(i, id_sock)] = OFFLINE;

                        // print_tables(id_sock, user_subscription, queue_msg, active_users);

                        // se scoate din multimea de citire socketul inchis
                        FD_CLR(i, &read_fds);
                        close(i);

                        cout << user_id << " has left.\n";
                        break;
                    }

                    memset(&message, 0, sizeof(message));
                    memcpy(&message, buffer, sizeof(message));
                    //printf("command: %d, topic: %s, sf: %d\n", message.type, message.topic, message.sf);

                    topic = message.topic;
                    user_id = get_id_for_socket(i, id_sock);

                    if (message.type == SUBSCRIBE) {
                        news_status news;

                        bool remaining_msg = subscribe(id_sock, user_subscription, queue_msg,
                                                       active_users, news, user_id, message.topic, message.sf);

                        if (remaining_msg) {
                            send_stored_messages(id_sock, user_subscription, queue_msg, user_id, topic);
                        }
                    } else if (message.type == UNSUBSCRIBE) {
                        unsubscribe(id_sock, user_subscription, queue_msg, active_users, user_id, topic);
                    }

                    // print_tables(id_sock, user_subscription, queue_msg, active_users);
                }
            }
        }
    }

    id_sock.clear();
    user_subscription.clear();
    queue_msg.clear();
    active_users.clear();
    close(tcp_sock);
    close(udp_sock);

    return 0;
}
