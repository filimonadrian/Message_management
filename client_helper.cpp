#include "client_helper.h"

void usage(char *file) {
    fprintf(stderr,
           "Usage: %s client_id(10 caract) server_address server_port\n", file);
    exit(0);
}

bool tokenize_command(char buffer[], user_msg &message) {
    char *token = strtok(buffer, " ");
    int i = 0;
    while (token != NULL) {
        if (i == 0) {
            if (!strcmp(token, "subscribe")) {
                message.type = SUBSCRIBE;
            } else if (!strcmp(token, "unsubscribe")) {
                message.type = UNSUBSCRIBE;
            } else {
                // printf("Incorrect subscribe \n");
                return false;
            }
        } else if (i == 1) {
            if (strlen(token) > 50) {
                return false;
            }

            strcpy(message.topic, token);

            /* if message is UNSUBSCRIBE, delete the last character(\n) */
            if (message.type == UNSUBSCRIBE) {
                message.topic[strlen(token) - 1] = '\0';
            } else {
                if (token[strlen(token) - 1] == '\n') {
                    return false;
                }
            }

        } else if (i == 2 && message.type == SUBSCRIBE) {
            if (!strcmp(token, "0\n")) {
                message.sf = SF_OFF;
            } else if (!strcmp(token, "1\n")) {
                message.sf = SF_ON;
            } else {
                // printf("Incorrect sf\n");
                return false;
            }
        }
        i++;
        token = strtok(NULL, " ");
    }
    return true;
}

bool print_received_message(tcp_msg message) {

    int int_num = 0;
    double real_num;

    switch (message.type) {
    case INT:
        /* it's not a sign byte */
        if (message.payload[0] > 1) {
            return false;
        }
        /* first byte is for sign */
        int_num = ntohl(*(uint32_t *)(message.payload + 1));

        if (message.payload[0] == 1) {
            int_num *= -1;
        }
        cout << message.udp_ip << ":" << message.udp_port << " - ";
        cout << message.topic << " - "
             << "INT"
             << " - " << int_num;
        cout << endl;

        break;

    case SHORT_REAL:
        real_num = ntohs(*(uint16_t *)message.payload);
        real_num /= 100;
        cout << message.udp_ip << ":" << message.udp_port << " - ";
        cout << message.topic << " - "
             << "SHORT_REAL"
             << " - " << real_num;
        cout << endl;

        break;

    case FLOAT:
        if (message.payload[0] > 1) {
            return false;
        }

        /* first byte is for sign */
        real_num = ntohl(*(uint32_t *)(message.payload + 1));
        real_num /= pow(10, message.payload[5]);

        if (message.payload[0] == 1) {
            real_num *= -1;
        }

        cout << message.udp_ip << ":" << message.udp_port << " - ";
        cout << message.topic << " - "
             << "FLOAT"
             << " - ";
        printf("%f\n", real_num);
        break;

    case STRING:
        cout << message.udp_ip << ":" << message.udp_port << " - ";
        cout << message.topic << " - "
             << "STRING"
             << " - " << message.payload;
        cout << endl;
        break;
    }
    return true;
}
