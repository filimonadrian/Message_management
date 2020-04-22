#include "server.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

static inline void clear_input_buffer() {
	char c = 0;
	while ((c = getchar()) != '\n'){}
}


int main(int argc, char *argv[]) {

	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// deschide socket TCP
	// trebuie sa deschid si socket UDP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Probleme cu crearea socketului");
	}

	portno = atoi(argv[1]);
	if (portno < 0) {
		perror("Problema cu portul");
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	int enable = 1;

// this socket is always free(for repeted tests)
 if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
   perror("setsocketopt");
   exit(1);
 }
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	if (ret < 0) {
		perror("Probleme bind");
	}

	ret = listen(sockfd, MAX_CLIENTS);
	if (ret < 0) {
		perror("Probleme listen");
	}

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	// se adauga file descriptor-ul pentru citirea de la stdin
	FD_SET(0, &read_fds);

	fdmax = sockfd;

	mesaj msg[15];
	tcp_client tcp_subscribers[15];
		int j = 0;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if (ret < 0) {
			perror("Probleme select");
		}

		if (FD_ISSET(0, &tmp_fds)) {
			char exit_message[10];
			memset(exit_message, 0, 10);
			fgets(exit_message, 6, stdin);
			
			if (exit_message[5] == 0 && strncmp(exit_message, "exit", 4) == 0){
				printf("Exiting..\n");
				break;
			} else {
				printf("That is not a valid command -_- \n");
				continue;
			}
			clear_input_buffer();

		}

		for (i = 0; i <= fdmax; i++) {
	
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					
					if (newsockfd < 0) {
						perror("Probleme cu accept-ul");
					}

					tcp_subscribers[j].sockfd = newsockfd;
					msg[j].name = (int)newsockfd;
					msg[j].sockfd = newsockfd;


					n = recv(newsockfd, buffer, sizeof(buffer), 0);
					if (n < 0) {
						perror("Can't receive user id!");
					}
					
					tcp_msg m;
					memcpy(&m, buffer, sizeof(tcp_msg));
					strcpy(tcp_subscribers[j].client_id, m.client_id);

					j++;
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}


					printf("New client %s, connected from %d:%s\n",
							tcp_subscribers[j - 1].client_id, ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr));
				} else {
					// s-au primit date pe unul din socketii de client,
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					if (n < 0) {
						perror("Receive topics problems");
					}

					int send_msg;

					//	trimit mesajul catre buffer[0]
					// printf("incerc sa caut clientul ce trebuie sa primeasca mesajul\n");
					for (int k = 0; k < j; k++){
						printf("name: %d, socket: %d\n", msg[k].name, msg[k].sockfd);
						
						if (msg[k].name == (buffer[0] - 48)){
							printf("trimit mesajul la %d %d\n", msg[k].name, msg[k].sockfd);
							send_msg = send(msg[k].name, buffer, strlen(buffer), 0);
							break;
						}
						
//aici ar trebui sa modific 
						// if (strcmp(tcp_subscribers[k].client_id, buffer[0])) {
						// 	printf("Send message to %s ", tcp_subscribers[k].client_id);
						// 	send_msg = send(tcp_subscribers[k].sockfd, buffer, strlen(buffer), 0);
						// 	break;
						// }
					}

					if (send_msg < 0) {
						perror("send_message to client");
					}
					
					if (n == 0) {
						// conexiunea s-a inchis
						printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(i);
						
						// se scoate din multimea de citire socketul inchis 
						FD_CLR(i, &read_fds);
					} else {
						printf ("clientul_%d: %s \n", i, buffer);
					}
				}
			}
		}
	}

	close(sockfd);

	return 0;
}
