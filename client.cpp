#include "client_helper.h"

int main(int argc, char *argv[]) {
    int sockfd, ret, received_data, send_test;;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    tcp_msg recv_message;
    user_msg message;
    bool is_correct = false;

    if (argc < 4) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Can't create socket");
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    ret = inet_aton(argv[2], &serv_addr.sin_addr);
    if (ret < 0) {
        perror("Ip connection problem");
    }

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (ret < 0) {
        perror("Can't connect to the server");
    }

    /*after connection, send the user_id to the server */
    memset(buffer, 0, BUFLEN);
    memcpy(buffer, argv[1], strlen(argv[1]));

    send_test = send(sockfd, buffer, sizeof(buffer), 0);

    fd_set read_fds; // multimea de citire folosita in select()
    fd_set tmp_fds;  // multime folosita temporar
    int fdmax;       // valoare maxima fd din multimea read_fds
    fdmax = sockfd;

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    tmp_fds = read_fds;

    while (1) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("Can't make select");
        }

        if (FD_ISSET(0, &tmp_fds)) {

            // read from stdin
            memset(buffer, 0, BUFLEN);
            memset(&message, 0, sizeof(message));
            fgets(buffer, BUFLEN - 1, stdin);

            if (!strcmp(buffer, "exit\n")) {
                break;
            } else {
                is_correct = tokenize_command(buffer, message);
                /* if input is incorrect */
                if (!is_correct) {
                    continue;
                }
            }

            printf("command: %d, topic: %s, sf: %d\n", message.type, message.topic, message.sf);

            // send message to server
            memset(buffer, 0, BUFLEN);
            memcpy(buffer, &message, sizeof(message));
            send_test = send(sockfd, buffer, sizeof(buffer), 0);
            if (send_test < 0) {
                perror("Can't send message");
            }
        }

        if (FD_ISSET(sockfd, &tmp_fds)) {

            received_data = recv(sockfd, buffer, sizeof(buffer), 0);
            if (received_data == 0) {
                printf("Server has closed unexpectedly. Exiting..\n");
                break;
            }
            if (received_data < 0) {
                perror("Can't receive message from server");
            }

            memset(&recv_message, 0, sizeof(tcp_msg));
            memcpy(&recv_message, buffer, sizeof(recv_message));

            // printf("%d:%d - %s - %s - %s", recv_message.udp_ip, recv_message.udp_port, recv_message.topic, );
            // printf("rec: %s \n", buffer);
        }
    }

    close(sockfd);

    return 0;
}

void decode_message() {

}