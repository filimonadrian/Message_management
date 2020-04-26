
// #include "server.h"
#include "server_helper.h"
/* functions to:
if topic it s new, add new topic and client
if topic is already knwown, add the user at the end of user_list
*/

void close_all_sock(fd_set *fd, int fdmax) {
    for (int sock = 0; sock < fdmax; sock++) {
        if (FD_ISSET(sock, fd)) {
            close(sock);
        }
    }
}

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

bool close_server(fd_set read_fds, int fdmax) {
    char exit_message[10];
    memset(exit_message, 0, 10);
    fgets(exit_message, 6, stdin);
    if (!strcmp(exit_message, "exit\n")) {
        close_all_sock(&read_fds, fdmax);
        printf("Exiting..\n");
        return true;
    }
    clear_input_buffer();
    printf("That is not a valid command -_- \n");
    return false;
}

void clear_input_buffer() {
    char c = 0;
    while ((c = getchar()) != '\n') {
    }
}

/*user_id for this socket*/
string get_id_for_socket(int sock, unordered_map<string, int> id_sock) {
    for (auto element : id_sock) {
        if (element.second == sock) {
            return element.first;
        }
    }

    return NULL;
}

/*used to delete a socket from vector(active_users) */
int get_index_of_socket(vector<int> &arr, int l, int r, int sock) {
    if (r >= l) {
        int mid = l + (r - l) / 2;
        if (arr[mid] == sock)
            return mid;

        if (arr[mid] > sock)
            return get_index_of_socket(arr, l, mid - 1, sock);

        return get_index_of_socket(arr, mid + 1, r, sock);
    }

    return -1;
}

/*index of topic from user_subscription */
int get_index_of_topic(unordered_map<string, vector<news_status>> &user_subscription,
                       string user_id, string topic) {
    int index = 0;
    for (auto element : user_subscription[user_id]) {
        if (element.topic == topic) {
            return index;
        }
        index++;
    }

    return -1;
}

void disconnect_user(unordered_map<string, int> &id_sock,
                     unordered_map<string, vector<news_status>> &user_subscription,
                     unordered_map<string, vector<tcp_msg>> &queue_msg,
                     unordered_map<string, vector<int>> &active_users,
                     int sockfd) {
    // set the status of client to offline
    string id = get_id_for_socket(sockfd, id_sock);
    if (id.size() == 0) {
        perror("Can't find id - request to exit");
    }
    /* for all topics of the user(user_subscription) */
    for (auto &element : user_subscription[id]) {
        /*user who disconects has all messages send */
        element.last_msg = queue_msg[element.topic].size() - 1;
        /*sort the vector */
        sort(active_users[element.topic].begin(), active_users[element.topic].end());
        /*apply binary search - delete socket of the user for every topic */
        int index = get_index_of_socket(active_users[element.topic],
                                        0, active_users[element.topic].size() - 1, sockfd);
        active_users[element.topic].erase(active_users[element.topic].begin() + index);
    }
}

/* subscribe an user and return true if there are messages to send */
/*add the topic in user_subscription and the sockfd in active_users*/
bool subscribe(unordered_map<string, int> &id_sock,
               unordered_map<string, vector<news_status>> &user_subscription,
               unordered_map<string, vector<tcp_msg>> &queue_msg,
               unordered_map<string, vector<int>> &active_users,
               news_status &news, string user_id, string topic, int sf) {

    int index = 0;

    /* is already subscribed? */
    /* if index > 0 => it is already subscribed */
    index = get_index_of_topic(user_subscription, user_id, topic);
    if (index > 0) {
        return false;
    }

    /*check if topic already exists */
    if (queue_msg.find(topic) == queue_msg.end()) {

        queue_msg.emplace(topic, vector<tcp_msg>());
        active_users.emplace(topic, vector<int>());
    }

    /*make user available to receive message for this topic */
    active_users[topic].push_back(id_sock[user_id]);

    news.last_msg = 0;
    news.sf = sf;
    news.topic = topic;
    user_subscription[user_id].push_back(news);

    return true;
}

/* remove the topic from user_subscription and the socket from socket active_users */
void unsubscribe(unordered_map<string, int> &id_sock,
                 unordered_map<string, vector<news_status>> &user_subscription,
                 unordered_map<string, vector<tcp_msg>> &queue_msg,
                 unordered_map<string, vector<int>> &active_users,
                 string user_id, string topic) {

    int index = 0;
    /* check if user is subscribed to this topic */

    index = get_index_of_topic(user_subscription, user_id, topic);
    if (index < 0) {
        return;
    }

    /* delete this topic for user */
    user_subscription[user_id].erase(user_subscription[user_id].begin() + index);

    /*delete the user's sockfd for this topic */
    index = get_index_of_socket(active_users[topic], 0,
                                active_users[topic].size() - 1, id_sock[user_id]);

    if (index < 0) {
        return;
    }
    active_users[topic].erase(active_users[topic].begin() + index);
}
/* send remaining messages for a topic if has SF_ON */
void send_stored_messages(unordered_map<string, int> &id_sock,
                          unordered_map<string, vector<news_status>> &user_subscription,
                          unordered_map<string, vector<tcp_msg>> &queue_msg,
                          string user_id, string topic) {

    int last_message, index = 0;
    int len = queue_msg[topic].size();
    int test_send;
    char buffer[BUFLEN];

    index = get_index_of_topic(user_subscription, user_id, topic);
    if (index < 0) {
        return;
    }

    if (user_subscription[user_id][index].sf == SF_OFF)
        return;

    last_message = user_subscription[user_id][index].last_msg;
    for (int i = last_message; i < len; i++) {
        memset(buffer, 0, BUFLEN);
        memcpy(buffer, &queue_msg[topic][i], sizeof(tcp_msg));
        test_send = send(id_sock[user_id], buffer, sizeof(buffer), 0);
        if (test_send < 0) {
            perror("Can't send stored messages");
        }
    }
    user_subscription[user_id][index].last_msg = len;
}

void print_tables(unordered_map<string, int> id_sock,
                  unordered_map<string, vector<news_status>> user_subscription,
                  unordered_map<string, vector<tcp_msg>> queue_msg,
                  unordered_map<string, vector<int>> active_users) {
    cout << "id_sock:\n";
    for (auto element : id_sock) {
        cout << element.first << " " << element.second << endl;
    }
    cout << endl
         << endl;
    cout << "user_subscription:\n";

    for (auto element : user_subscription) {
        cout << element.first << ": ";
        for (auto el : element.second) {
            cout << el.last_msg << " " << el.sf << " " << el.topic << " -> ";
            // printf("%d %d -> ", el.last_msg, el.sf);
        }
        cout << endl;
    }

    cout << endl
         << endl;
    cout << "queue_msg:\n";

    for (auto element : queue_msg) {
        cout << element.first << ": ";
        for (auto el : element.second) {
            //cout << el.topic << " " << el.type << " " << el.payload << " -> ";
            printf("%s %d -> ", el.topic, el.type);
        }
        cout << endl;
    }
    cout << endl
         << endl;
    cout << "active_users:\n";

    for (auto element : active_users) {
        cout << element.first << ": ";
        for (auto el : element.second) {
            cout << el << " -> ";
        }
        cout << endl;
    }
    cout << endl
         << endl;
}