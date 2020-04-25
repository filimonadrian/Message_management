#include "client_helper.h"

void usage(char *file) {
    fprintf(stderr, "Usage: %s client_id(10 caract) server_address server_port\n", file);
    exit(0);
}

bool tokenize_command(char buffer[], user_msg &message) {
/*** daca e unsubscribe nu trebuie sa mai astept sf-ul ***/
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
                // printf("Incorrect topic \n");
                return false;
            }
            /* copy first TOPIC_LEN - 1 characters (first 50 -->0 - 49) */
            strncpy(message.topic, token, TOPIC_LEN - 1);
            /* the last character will be \0 */
            message.topic[TOPIC_LEN - 1] = '\0';

        } else if (i == 2 && message.type == SUBSCRIBE) {
            if (!strcmp(token, "0\n")) {
                message.sf = 0;
            } else if (!strcmp(token, "1\n")) {
                message.sf = 1;
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