#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>

#include "rdt.h"

#define FILA 1

int main(int argc, char **argv) {
	struct timeval tout;
	tout.tv_sec=3;
    tout.tv_usec=0;

	// ./server <porta>
	if (argc !=2) {
		printf("%s <porta>\n", argv[0]);
		return 0;
	}

	struct tparam_t t[FILA];

	for(int i=0; i<FILA; i++){
		struct sockaddr_in addr;
		addr.sin_addr.s_addr = INADDR_ANY;

		printf("porta: %d\n", atoi(argv[1]) + i);

		addr.sin_port = htons(atoi(argv[1]) + i);
		addr.sin_family = AF_INET;

		t[i].cfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (t[i].cfd == -1) {
			perror("socket()");
			return -1;
		}
		if(FILA > 1)
			if( setsockopt(t[i].cfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tout, sizeof(struct timeval)) != 0){
				perror("setsockopt failed"); 
				exit(EXIT_FAILURE); 
			}

		t[i].oncethru = 0;
		t[i].state = 0;
		if (bind(t[i].cfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
			perror("bind()");
			return -1;
		}
	}

	int i=0;

	while (1) {
		printf("socket id = %d\n", i);

		t[i].addr_len = sizeof(struct sockaddr_in);
		bzero(&t[i].caddr, t[i].addr_len);
		if (t[i].cfd == -1) {
			perror("accept()");
			close(t[i].cfd);
			return -1;
		}

		rdt_rcv((void *)&t[i]);

		i = (i+1)%FILA;
	}
	for(int i=0; i<FILA; i++){
		close(t[i].cfd);
	}
	return 0;
}