#include "server.h"
#include "server_helper.h"

void print_tables(unordered_map<string, int> id_sock,
                  unordered_map<string, vector<news_status>> user_subscription,
                  unordered_map<string, vector<tcp_msg>> queue_msg,
                  unordered_map<string, vector<int>> active_users) {
    cout << "id_sock:\n";
    for (auto element : id_sock) {
        cout << element.first << " " << element.second << endl;
    }
    cout << endl << endl;
    cout << "user_subscription:\n";

// pune sf ul in structura !!!

    for (auto element : user_subscription) {
        cout << element.first << " ";
        for (auto el : element.second) {
            cout << el.last_msg << " " << el.topic << " " << el.sf << "__";
        }
    }


    cout << endl<< endl;
    cout << "queue_msg:\n";

    for (auto element : queue_msg) {
        cout << element.first << " ";
        for (auto el : element.second) {
            cout << el.topic << " " << el.type << " " << el.payload << "__";
        }
    }
    cout << endl<<endl;
    cout << "active_users:\n";

    for (auto element : active_users) {
        cout << element.first << " ";
        for (auto el : element.second) {
            cout << el << "__";
        }
    }
    cout << endl << endl;
}

int main(int argc, char *argv[]) {

    int tcp_sock, udp_sock, newsockfd, portno;
    char buffer[BUFLEN];
    struct sockaddr_in tcp_addr, udp_addr, from_station;
    socklen_t source_len;
    string topic, user_id;
    user_msg message;

    int i, ret;
    int received_data;
    // vector<tcp_client> tcp_users;

    /* <user_id> and <socket> to update file descriptor*/
    unordered_map<string, int> id_sock;

    /* <user_id> and all associated topics */
    // unordered_map<string, string> users_topic;

    /*<user_id> and <vector_news> with topic, sf, and index of last send message*/
    /* used to check if user wants all informations about topic; keep last message sent */
    unordered_map<string, vector<news_status>> user_subscription;

    /*<topic_name> and vector of messages ready to send*/
    unordered_map<string, vector<tcp_msg>> queue_msg;

    /*all active connections for a topic */
    unordered_map<string, vector<int>> active_users;

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar
    int fdmax;       // valoare maxima fd din multimea read_fds

    if (argc < 2) {
        usage(argv[0]);
    }

    // se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
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

    ret = bind(udp_sock, (struct sockaddr *)&udp_addr, sizeof(struct sockaddr));
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

    fdmax = tcp_sock;

    mesaj msg[15];

    tcp_client tcp_subscribers[15];
    tcp_client user;

    int j = 0;

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("Probleme select");
        }

        if (FD_ISSET(0, &tmp_fds)) {
            if (close_server(read_fds, fdmax)) {
                break;
            } else {
                continue;
            }
            //clear_input_buffer();
        }

        for (i = 1; i <= fdmax; i++) {

            if (FD_ISSET(i, &tmp_fds)) {
                if (i == udp_sock) {

                    memset(buffer, 0, BUFLEN);
                    source_len = sizeof(from_station);
                    received_data = recvfrom(udp_sock, buffer, BUFLEN, 0, (struct sockaddr *)&from_station, &source_len);
                    if (received_data < 0) {
                        perror("Can't receive udp message");
                    }
                    /* unwrap the udp message */
                    udp_msg message;
                    tcp_msg ready_to_send;
                    memcpy(&message, buffer, sizeof(message));
                    ready_to_send.udp_port = ntohs(from_station.sin_port);
                    /*ip is uint32_t !!!! */
                    ready_to_send.udp_ip = ntohl(from_station.sin_addr.s_addr);

                    strcpy(ready_to_send.topic, message.topic);
                    strcpy(ready_to_send.payload, message.payload);
                    ready_to_send.type = message.type;

                    string topic = message.topic;

                    /* if it's a new topic, create new entry in queue and in active_users */
                    if (queue_msg.find(topic) == queue_msg.end()) {
                        queue_msg.emplace(topic, vector<tcp_msg>());

                        active_users.emplace(topic, vector<int>());
                    } else {
                        /* if topic exists, send message to active users and
                        push message to the msg_queue(for other users) */
                        queue_msg[topic].push_back(ready_to_send);
                        /* send to all users */
                        int check_send, len = active_users[topic].size();
                        memcpy(buffer, &ready_to_send, sizeof(ready_to_send));
                        for (int k = 0; k < len; k++) {
                            check_send = send(active_users[topic][k], buffer, BUFLEN, 0);
                            if (check_send < 0) {
                                perror("Can't send message to active user");
                            }
                        }
                    }

                } else if (i == tcp_sock) {

                    /* new connection request 
						server accepts the request */
                    source_len = sizeof(from_station);
                    newsockfd = accept(tcp_sock, (struct sockaddr *)&from_station, &source_len);

                    if (newsockfd < 0) {
                        perror("Can't accept new clients!");
                    }

                    /* Now i need the user id*/

                    /*clean buffer*/
                    memset(buffer, 0, BUFLEN);
                    received_data = recv(newsockfd, buffer, sizeof(buffer), 0);
                    if (received_data < 0) {
                        perror("Can't receive user id!");
                    }
                    /*unwrap the message */
                    /*The buffer contains just THE USER ID */
                    //!!!!- probably 2 long user id
                    string user_id = buffer;
                    /*useless --> just for testing */
                    strcpy(tcp_subscribers[j].client_id, buffer);

                    /*if user_id doesn't exist, create a new entry */
                    if (id_sock.find(user_id) == id_sock.end()) {

                        id_sock.insert({user_id, newsockfd});
                        user_subscription.insert(make_pair(user_id, vector<news_status>()));
                        /*same shit */
                        user_subscription.emplace(user_id, vector<news_status>());

                        /*else the user_id is back online -->update sockfd, tick as active user, send messages */
                    } else {
                        id_sock[user_id] = newsockfd;
                        /*send each message which has sf on */
                        /* for each topic of this new user */
                        for (auto element : user_subscription[user_id]) {
                            /*tick as active user for current topic */
                            (active_users[element.topic]).push_back(newsockfd);
                            if (element.sf == SF_ON) {
                                /* send last messages */
                                int len = queue_msg[element.topic].size();
                                for (int k = element.last_msg; k < len; k++) {
                                    memset(buffer, 0, BUFLEN);
                                    vector<tcp_msg> msg = queue_msg[element.topic];
                                    memcpy(buffer, &msg[i], BUFLEN);
                                    int s = send(newsockfd, buffer, BUFLEN, 0);
                                    if (s < 0) {
                                        perror("Can't send stored messages");
                                    }
                                }
                            }
                        }
                    }

                    tcp_subscribers[j].sockfd = newsockfd;

                    msg[j].name = (int)newsockfd;
                    msg[j].sockfd = newsockfd;

                    j++;
                    // se adauga noul socket intors de accept() la multimea descriptorilor de citire
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    cout << "New client " << user_id << " connected from " << ntohs(from_station.sin_port);
                    cout << ":" << inet_ntoa(from_station.sin_addr) << endl;

                    print_tables(id_sock, user_subscription, queue_msg, active_users);

                } else {
                    /* i've received data on an existing socket
						the server must interpret them */
                    memset(buffer, 0, BUFLEN);
                    received_data = recv(i, buffer, sizeof(buffer), 0);
                    if (received_data < 0) {
                        perror("Receive TCP messages problem");
                    }

                    memset(&message, 0, sizeof(message));
                    memcpy(&message, buffer, sizeof(message));
                    printf("command: %d, topic: %s, sf: %d\n", message.type, message.topic, message.sf);

                    topic = message.topic;
                    user_id = get_id_for_socket(i, id_sock);

                    if (message.type == SUBSCRIBE) {
                        news_status news;

                        bool remaining_msg = subscribe(id_sock, user_subscription, queue_msg,
                                                       active_users, news, user_id, message.topic, message.sf);
                        if (!remaining_msg) {
                            send_stored_messages(id_sock, user_subscription, queue_msg, user_id, topic);
                        }
                    } else if (message.type == UNSUBSCRIBE) {
                        unsubscribe(id_sock, user_subscription, queue_msg, active_users, user_id, topic);
                    }

                    print_tables(id_sock, user_subscription, queue_msg, active_users);

                    if (received_data == 0) {
                        // the connection has ended
                        //problems with cout
                        cout << "Clientul " << get_id_for_socket(i, id_sock) << " a inchis conexiunea";
                        //printf("Clientul %s a inchis conexinea\n", get_id_for_socket(i, id_sock));
                        //printf("Socket-ul client %d a inchis conexiunea\n", i);

                        disconnect_user(id_sock, user_subscription, queue_msg, active_users, i);
                        close(i);

                        // se scoate din multimea de citire socketul inchis
                        FD_CLR(i, &read_fds);
                    } else {
                        // printf("clientul_%d: %s \n", i, buffer);
                    }
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
