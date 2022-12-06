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
    for(i=4;i<sizeof(msg);i++){
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
	printf("AAAAA");
	checksum.dword = get_checksum(req);
	if(checksum.byte0 != req[0]) return 1;
	if(checksum.byte1 != req[1]) return 1;
	if(checksum.byte2 != req[2]) return 1;
	if(checksum.byte3 != req[3]) return 1;

	return 0;
}
	

int main(int argc, char **argv) {
	// ./server <porta>
	if (argc !=2) {
		printf("%s <porta>\n", argv[0]);
		return 0;
	}
	printf("AAAA");

	union CkSum Ack0, Ack1;
	Ack0.dword =~ 0;
	Ack1.dword =~ 1;
	char pktAck[2][6] = {0};


	sprintf(pktAck[1],"%c%c%c%c1", Ack0.byte0,Ack0.byte1,Ack0.byte2,Ack0.byte3);
	sprintf(pktAck[0],"%c%c%c%c0", Ack1.byte0,Ack1.byte1,Ack1.byte2,Ack1.byte3); 

	printf("AAAA");

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

	for(int i=0; i<FILA; i++){
		t[i].oncethru = 0;
		t[i].state = 0;
	}

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
		t[i].nr = recvfrom(t[i].cfd, t[i].req, MAXLINE, 0,
					(struct sockaddr *)&t[i].caddr, &t[i].addr_len);
		
		if(isCorrupt(t[i].req) || (isWrongState(t[i]))){
			if(t[i].oncethru == 1){
				sendto(t[i].cfd, pktAck[(t[i].state+1)%2], 6, 0, (struct sockaddr*)&t[i].caddr, sizeof(struct sockaddr_in));
			}
		}else{
			pthread_create(&t[i].tid, NULL, trata_cliente, (void *)&t[i]);
			sendto(t[i].cfd, pktAck[t[i].state], 6 , 0, (struct sockaddr*)&t[i].caddr, sizeof(struct sockaddr_in));
			t[i].oncethru = 1;
			t[i].state = (t[i].state+1)%2;
		}

		i = (i+1)%FILA;
		break;
	}
	close(ls);
	return 0;
}