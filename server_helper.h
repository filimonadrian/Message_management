#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include "general_includes.h"
#include "server.h"

void close_all_sock(fd_set *fd, int fdmax);
void usage(char *file);
bool close_server(fd_set read_fds, int fdmax);
void clear_input_buffer();
string get_id_for_socket(int sock, unordered_map<string, int> id_sock);

int get_index_of_socket(vector<int> &arr, int l, int r, int sock);
int get_index_of_topic(unordered_map<string, vector<news_status>> &user_subscription, 
                        string user_id, string topic);

void disconnect_user(unordered_map<string, int> &id_sock,
                     unordered_map<string, vector<news_status>> &user_subscription,
                     unordered_map<string, vector<tcp_msg>> &queue_msg,
                     unordered_map<string, vector<int>> &active_users,
                     int sockfd);

bool subscribe(unordered_map<string, int> &id_sock,
               unordered_map<string, vector<news_status>> &user_subscription,
               unordered_map<string, vector<tcp_msg>> &queue_msg,
               unordered_map<string, vector<int>> &active_users,
               news_status &news, string user_id, string topic, int sf);

void unsubscribe(unordered_map<string, int> &id_sock,
                 unordered_map<string, vector<news_status>> &user_subscription,
                 unordered_map<string, vector<tcp_msg>> &queue_msg,
                 unordered_map<string, vector<int>> &active_users,
                 string user_id, string topic);

void send_stored_messages(unordered_map<string, int> &id_sock,
                          unordered_map<string, vector<news_status>> &user_subscription,
                          unordered_map<string, vector<tcp_msg>> &queue_msg,
                          string user_id, string topic);

void print_tables(unordered_map<string, int> id_sock,
                  unordered_map<string, vector<news_status>> user_subscription,
                  unordered_map<string, vector<tcp_msg>> queue_msg,
                  unordered_map<string, vector<int>> active_users);

#endif