#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

#define SUBSCRIBE 1
#define UNSUBSCRIBE 0
#define ID_ACKNOWLEDGE 2
#define SF_ON 1
#define SF_OFF 0

void usage(char *file) {
	fprintf(stderr, "Usage: %s client_id(10 caract) server_address server_port\n", file);
	exit(0);
}

typedef struct tcp_msg{
	char client_id[11];
	char payload[100];
	uint8_t type;
	uint8_t SF;
}tcp_msg;

int main(int argc, char *argv[]) {
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char message [BUFLEN];
	char client_id [11];

	if (argc < 4) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");



// imediat dupa ce ma conectez trebuie sa trimit si un mesaj de recunoastere a id-ului
	tcp_msg send_id;
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	strcpy(send_id.client_id, argv[1]);
	send_id.type = ID_ACKNOWLEDGE;
	memcpy(buffer, &send_id, sizeof(tcp_msg));
	n = send(sockfd, buffer, strlen(buffer), 0);
	

	

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds
	fdmax = sockfd;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	tmp_fds = read_fds;

	while (1) {
		tmp_fds = read_fds;
		memset(message, 0, BUFLEN);
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		if (FD_ISSET(0, &tmp_fds)) {

			// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}
			// se trimite mesaj la server
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");
		}

		if (FD_ISSET(sockfd, &tmp_fds)) {
			
			int rec_msg = recv(sockfd, message, sizeof(message), 0);
			if (rec_msg == 0) {
				printf("Server has closed unexpectedly. Exiting..\n");
				close(sockfd);
				break;
			}
			DIE(rec_msg < 0, "receive message from server");
			printf("rec: %s \n", message);
			}

		}
	

	close(sockfd);

	return 0;
}
