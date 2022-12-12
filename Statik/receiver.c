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

#define FILA 1
#define MAXLINE 1000
#define STATE 4

struct tparam_t {
	int cfd, nr;
	pthread_t tid;
	struct sockaddr_in caddr;
	socklen_t addr_len;
	char req[MAXLINE];
	char state;
	char oncethru;
};

int get_checksum(char *msg){
    int checksum, sum=0, i;
    for(i=4;i<strlen(msg);i++){
		if(msg[i] == 0) break;
        sum+=msg[i];
    }
    checksum=~sum;
    return checksum;
}
union CkSum
{
    int dword;

    struct
    {
        uint8_t byte0;
        uint8_t byte1;
        uint8_t byte2;
        uint8_t byte3;
    };
};

void printpacket(char *msg){
    for(int i=0;i<50;i++){
        // printf("%c // %d    ",msg[i],msg[i]);
        // printf("%c ",msg[i]);
        printf("%d ",msg[i]);
    }
	printf("\n");
}
void *trata_cliente(void *args){
	struct tparam_t t = *(struct tparam_t*)args;
	printf("Cliente IP(%s):Porta(%d): %d bytes: %s \n",
		inet_ntoa(t.caddr.sin_addr),
		ntohs(t.caddr.sin_port),
		t.nr,
		t.req);

	fflush(stdout);
	// processa
	pthread_exit(NULL);
}
int isWrongState(struct tparam_t t){
	if(t.req[STATE] == t.state + 48) return 0;
	return 1;
}

int isCorrupt(char *req){
	union CkSum checksum;
	int i;

	checksum.dword = get_checksum(req);

    // printf("estou na funcao corrupto\n req:\n");
    // printpacket(req);
    // printf("checksum esperado:\n");
    // printf("\n%d %d %d %d\n",
    //     checksum.byte0,
    //     checksum.byte1,
    //     checksum.byte2,
    //     checksum.byte3
    // ); 
    // printf("\nchar byte0 %c char req0 %c \n int byte0 %d int req0 %d\n",
    //     checksum.byte0,
    //     req[0],
    //     checksum.byte0,
    //     req[0]
    // ); 

	if((char) checksum.byte0 != (char) req[0]) return 1;
	if((char) checksum.byte1 != (char) req[1]) return 1;
	if((char) checksum.byte2 != (char) req[2]) return 1;
	if((char) checksum.byte3 != (char) req[3]) return 1;

	return 0;
}
	

int main(int argc, char **argv) {
	struct timeval tout;

	// ./server <porta>
	if (argc !=2) {
		printf("%s <porta>\n", argv[0]);
		return 0;
	}
	tout.tv_sec=3;
    tout.tv_usec=0;

	union CkSum Ack0, Ack1;
	Ack0.dword =~ '0';
	Ack1.dword =~ '1';
	char pktAck[2][6] = {0};

	sprintf(pktAck[0],"%c%c%c%c0", Ack0.byte0,Ack0.byte1,Ack0.byte2,Ack0.byte3);
	sprintf(pktAck[1],"%c%c%c%c1", Ack1.byte0,Ack1.byte1,Ack1.byte2,Ack1.byte3); 

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
		t[i].nr = recvfrom(t[i].cfd, t[i].req, MAXLINE, 0,
					(struct sockaddr *)&t[i].caddr, &t[i].addr_len);
		
		t[i].req[t[i].nr] = 0;

		if(t[i].nr != -1){
            if(isCorrupt(t[i].req)) printf("Pacote corrompido\n");
            if(isWrongState(t[i])) printf("Pacote no estado errado\n");
			if((isCorrupt(t[i].req) || (isWrongState(t[i]))) && t[i].oncethru == 1){
				printf("entrei no is corrupt is wrongstate.\n");
				sendto(t[i].cfd, pktAck[(t[i].state+1)%2], 6, 0, (struct sockaddr*)&t[i].caddr, sizeof(struct sockaddr_in));
			}else{
				printf("vou tratar o cliente.\n");
				pthread_create(&t[i].tid, NULL, trata_cliente, (void *)&t[i]);
				printf("tratei o cliente.\n");
				sendto(t[i].cfd, pktAck[t[i].state], 6, 0, (struct sockaddr*)&t[i].caddr, sizeof(struct sockaddr_in));
				t[i].oncethru = 1;
				t[i].state = (t[i].state+1)%2;
			}
		}
		i = (i+1)%FILA;
	}
	for(int i=0; i<FILA; i++){
		close(t[i].cfd);
	}
	return 0;
}