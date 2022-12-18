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

#define STATE 4
#define MAXLINE 1000
#define tempoDinamico 1

/* Header do sndpkt
    1,2,3,4 bytes: checksum
    5 byte: state
    próximos bytes: mensagem
*/

struct tparam_t {
	int cfd, nr;
	pthread_t tid;
	struct sockaddr_in caddr;
	socklen_t addr_len;
	char req[MAXLINE];
	char state;
	char oncethru;
};

struct timeConfig {
    double EstRTT, DevRTT;
};

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
	printf("tamanho da msg: %ld\n",strlen(msg));
    for(int i=0;i<MAXLINE/*sizeof(msg)*/;i++){
		if(msg[i] == 0) break;
        //printf("%c // %d    ",msg[i],msg[i]);
        //printf("%c ",msg[i]);
        printf("%d ",msg[i]);
    }
	printf("\n");
}

int get_checksum(char *msg, int start){
    int checksum, sum=0, i;
    for(i=start;i<strlen(msg);i++){
		if(msg[i] == 0) break;
        sum+=msg[i];
    }
    checksum=~sum;
    return checksum;
}

void sender_make_pkt(char *pkt, char *msg, int state){
    union CkSum checksum;
    char aux[MAXLINE+1] = {0};
    memset(aux,0,sizeof(aux));

    aux[0] = state + 48;

    strcat(aux, msg);              
    checksum.dword = get_checksum(aux, 0);

    sprintf(pkt, "%c%c%c%c%d%s",
            checksum.byte0,
            checksum.byte1,
            checksum.byte2,
            checksum.byte3,
            state,msg
            ); 
}

int isWrongState(char *req, int state){
	if(req[STATE] == state + 48) return 0;
	return 1;
}

int isCorrupt(char *req, int start){
	union CkSum checksum;
	int i;

	checksum.dword = get_checksum(req, start);

    // printf("\n%d %d %d %d\n",        /// print dos bytes para o checksum
    //         checksum.byte0,
    //         checksum.byte1,
    //         checksum.byte2,
    //         checksum.byte3
    //         ); 

	if((char) checksum.byte0 != (char) req[0]) return 1;
	if((char) checksum.byte1 != (char) req[1]) return 1;
	if((char) checksum.byte2 != (char) req[2]) return 1;
	if((char) checksum.byte3 != (char) req[3]) return 1;

	return 0;
}

float convertTime(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) + 1e-6*(end.tv_usec - start.tv_usec);
}

void rdt_rcv(void *tparam_args){
    union CkSum Ack0, Ack1;
	Ack0.dword =~ '0';
	Ack1.dword =~ '1';
	char pktAck[2][6] = {0};
	sprintf(pktAck[0],"%c%c%c%c0", Ack0.byte0,Ack0.byte1,Ack0.byte2,Ack0.byte3);
	sprintf(pktAck[1],"%c%c%c%c1", Ack1.byte0,Ack1.byte1,Ack1.byte2,Ack1.byte3); 

    struct tparam_t *t = (struct tparam_t*)tparam_args;
    t->nr = recvfrom(t->cfd, t->req, MAXLINE, 0,
            (struct sockaddr *)&t->caddr, &t->addr_len);
    // printpacket(t->req);
    if(t->nr != -1){
        t->req[t->nr] = 0; // ultimo byte recebe /0]
        printf("Estado da Mensagem: %c\n",t->req[4]);
        printf("Mensagem recebida: %s\n\n",&t->req[5]);
        if(isCorrupt(t->req, 4)) printf("Pacote corrompido\n");
        if(isWrongState(t->req, t->state)) printf("Pacote no estado errado\n");
        if((isCorrupt(t->req, 4) || (isWrongState(t->req, t->state))) && t->oncethru == 1){
            printf("entrei no is corrupt is wrongstate.\n");
            sendto(t->cfd, pktAck[(t->state+1)%2], 6, 0, (struct sockaddr*)&t->caddr, sizeof(struct sockaddr_in));
        }else{
            sendto(t->cfd, pktAck[t->state], 6, 0, (struct sockaddr*)&t->caddr, sizeof(struct sockaddr_in));
            t->oncethru = 1;
            t->state = (t->state+1)%2;
        }
    }
}

void rdt_snd(int sockfd, char *msg, int state, char *port, char *ip,void *TConf){
    struct timeval tout;
    struct timeval start, end;
    struct timeConfig *tConf = (struct timeConfig*)TConf;
    int n, len = 0;
    char sndpkt[MAXLINE] = {1};
    double timeoutAux = (tConf->EstRTT + 4*tConf->DevRTT);
    tout.tv_usec = ((int)(1000000*timeoutAux))%1000000;
    tout.tv_sec = (int)timeoutAux;
    char rcvpkt[5]; 
    struct sockaddr_in servaddr;

    memset(&servaddr, 0, sizeof(servaddr)); 
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(atoi(port)); 
    servaddr.sin_addr.s_addr = inet_addr(ip);

    if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tout, sizeof(struct timeval)) != 0){
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }

    gettimeofday(&start, NULL);
    do{
        sender_make_pkt(sndpkt, msg, state);
        sendto(sockfd, (const char *)sndpkt, strlen(sndpkt), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                sizeof(servaddr));
        printf("\nMensagem \"%s\" enviada.\n",msg);
        // printpacket(sndpkt);

        n = recvfrom(sockfd, rcvpkt, 6,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len);
        rcvpkt[5] = 0; 
        if(n != -1){
            printf("Estado recebido: %c\n", rcvpkt[4]);
        }
        if(n == -1){
            printf("Timeout\n");
        }
        else{                
            if(isCorrupt(rcvpkt, 4)) printf("Ack corrompido\n");
        }
        // printpacket(rcvpkt);

    } while(n == -1 || isCorrupt(rcvpkt, 4) || isWrongState(rcvpkt, state));
    gettimeofday(&end, NULL);
    if(tempoDinamico == 1){
        // Ajuste do tempo de timeout
        printf("EST: %lf\n",tConf->EstRTT);
        double totalTime = convertTime(start, end);
        printf("TT: %f\n",totalTime);
        tConf->EstRTT = tConf->EstRTT*0.8 + totalTime*0.2;
        tConf->DevRTT = 0.75*tConf->DevRTT +(0.25*abs(totalTime-tConf->EstRTT)) ;
        timeoutAux = (tConf->EstRTT + 4*tConf->DevRTT);
        printf("EST: %lf\n",tConf->EstRTT);
        printf("\nTempo de demora do pacote: %f\nTempo de TimeOut Recalculado: %lf\n",totalTime,timeoutAux);
        if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tout, sizeof(struct timeval)) != 0){
            perror("setsockopt failed"); 
            exit(EXIT_FAILURE); 
        }
    }
}