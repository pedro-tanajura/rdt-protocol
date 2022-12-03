#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>

#define FILA 1000
#define MAX_REQ 1000

struct tparam_t {
	int cfd, nr;
	pthread_t tid;
	struct sockaddr_in caddr;
	socklen_t addr_len;
	char req[MAX_REQ];
};

void *trata_cliente(void *args){
	struct tparam_t t = *(struct tparam_t*)args;
	printf("Cliente IP(%s):Porta(%d): %d bytes: %s \n",
		inet_ntoa(t.caddr.sin_addr),
		ntohs(t.caddr.sin_port),
		t.nr,
		t.req);
	fflush(stdout);
	// processa
	sendto(t.cfd, t.req, t.nr, 0, (struct sockaddr*)&t.caddr, sizeof(struct sockaddr_in));
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
	// ./server <porta>
	if (argc !=2) {
		printf("%s <porta>\n", argv[0]);
		return 0;
	}

	int ls = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ls == -1) {
		perror("socket()");
		return -1;
	}

	struct sockaddr_in addr;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(atoi(argv[1]));
	addr.sin_family = AF_INET;
	if (bind(ls, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind()");
		return -1;
	}


	struct tparam_t t[FILA];
	int i=0;
	while (1) {
		t[i].addr_len = sizeof(struct sockaddr_in);
		bzero(&t[i].caddr, t[i].addr_len);
		if (t[i].cfd == -1) {
			perror("accept()");
			close(ls);
			return -1;
		}
		t[i].cfd = ls;
		t[i].nr = recvfrom(t[i].cfd, t[i].req, MAX_REQ, 0,
					(struct sockaddr *)&t[i].caddr, &t[i].addr_len);
		pthread_create(&t[i].tid, NULL, trata_cliente, (void *)&t[i]);
		i = (i+1)%FILA;
	}
	close(ls);
	return 0;
}