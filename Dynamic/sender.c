// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>

#define PORT    5000 
#define MAXLINE 1000
#define STATE 4

/* Header do sndpkt
    1,2,3,4 bytes: checksum
    5 byte: state
    próximos bytes: mensagem
*/

//////////////// EM PROGRESSO //////////////////// 
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
    for(int i=0;i<sizeof(msg);i++){
		if(msg[i] == 0) break;
        // printf("%c // %d    ",msg[i],msg[i]);
        // printf("%c ",msg[i]);
        printf("%d ",msg[i]);
    }
	printf("\n");
}

int get_checksum(char *msg, int start){
    int checksum, sum=0, i;
    //printf("msg %s\n",msg);
    //printf("strlen %d\n",strlen(msg));
    for(i=start;i<strlen(msg);i++){
		if(msg[i] == 0) break;
        sum+=msg[i];
    }
    checksum=~sum;
    return checksum;
}

void sender_make_pkt(char *pkt, char *msg, int state){
    // printf("entrei no sender mk pkt\n");

    union CkSum checksum;
    char aux[MAXLINE+1] = {0};
    memset(aux,0,sizeof(aux));

    aux[0] = state + 48;

    // printf("aux entre strcat: %s\n", aux);
    strcat(aux, msg);                  // Add msg
    // printf("aux: %s\n", aux);

    checksum.dword = get_checksum(aux, 0);
    // printf("Soma: %d ",checksum.dword);
    // printf("\n%d %d %d %d\n",
    //         checksum.byte0,
    //         checksum.byte1,
    //         checksum.byte2,
    //         checksum.byte3
    //         ); 
    sprintf(pkt, "%c%c%c%c%d%s",
            checksum.byte0,
            checksum.byte1,
            checksum.byte2,
            checksum.byte3,
            state,msg
            ); 
    // printpacket(pkt);
    // printf("\n\n");
}

int isWrongState(char *req, int state){
    printf("Estado recebido: %c - Estado esperado: %c\n", req[STATE], state + 48);
	if(req[STATE] == state + 48) return 0;
    printf("Pacote no estado errado\n");
	return 1;
}

int isCorrupt(char *req){
	union CkSum checksum;
	int i;
    
	checksum.dword = get_checksum(req, 4);
    /*
    printf("estou na funcao corrupto\n req:\n");
    printpacket(req);
    printf("checksum esperado:\n");
    printf("\n%d %d %d %d\n",
        checksum.byte0,
        checksum.byte1,
        checksum.byte2,
        checksum.byte3
        ); 
    printf("\nchar byte0 %c char req0 %c \n int byte0 %d int req0 %d\n",
        checksum.byte0,
        req[0],
        checksum.byte0,
        req[0]
        ); 
    */
	if((char) checksum.byte0 != (char) req[0]) return 1;
	if((char) checksum.byte1 != (char) req[1]) return 1;
	if((char) checksum.byte2 != (char) req[2]) return 1;
	if((char) checksum.byte3 != (char) req[3]) return 1;

	return 0;
}

/////////////////////////////////////////////////
float convertTime(struct timeval start, struct timeval end)
{
    return (end.tv_sec - start.tv_sec) + 1e-6*(end.tv_usec - start.tv_usec);
}

int main(int argc, char **argv) { 
    int sockfd; 
    char rcvpkt[5];  
    struct sockaddr_in servaddr; 
    struct timeval tout;
    double EstRTT, DevRTT;
    struct timeval start, end;

    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    tout.tv_sec=0;
    tout.tv_usec=2000000;
    EstRTT = 2;
    DevRTT = 0.05;
    if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tout, sizeof(struct timeval)) != 0){
        perror("setsockopt failed"); 
        exit(EXIT_FAILURE); 
    }
    
    memset(&servaddr, 0, sizeof(servaddr)); 
        
    printf("%s\n", argv[1]);
    printf("%s\n", argv[2]);
    printf("%s\n", argv[3]);
    // Filling server information 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(atoi(argv[2])); 
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
        
    int n, len, state = 0;
    char sndpkt[MAXLINE] = {1};
    char msg[MAXLINE] = {0};
    gettimeofday(&start, NULL);
    while(1){
        sprintf(msg, argv[3]); // Adicionar o valor state e checksum
        sender_make_pkt(sndpkt,msg,state);
        do{
            // rdt_send()
            sendto(sockfd, (const char *)sndpkt, strlen(sndpkt), 
                MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                    sizeof(servaddr));
            printf("\nMensagem \"%s\" enviada.\n",msg);
            
            //
        
            // rdt_rcv()
            n = recvfrom(sockfd, rcvpkt, 6,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 
            if(n != -1){
                printf("Estado recebido: %c\n", rcvpkt[4]);
            }
            //
            // sleep(3);
            if(n == -1){
                printf("Timeout\n");

            }
            else{                
                gettimeofday(&end, NULL);
                float totalTime = convertTime(start, end);
                EstRTT = EstRTT*0.8 + totalTime*0.2;
                DevRTT = 0.75*DevRTT +(0.25*(totalTime-EstRTT)) ;
                tout.tv_usec = 1000000*(EstRTT + 4*DevRTT);
                gettimeofday(&start, NULL);
                printf("\nTempo de TimeOut Recalculado: %ld\n",tout.tv_usec);
                if( setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tout, sizeof(struct timeval)) != 0){
                    perror("setsockopt failed"); 
                    exit(EXIT_FAILURE); 
                }
                if(isCorrupt(rcvpkt)) printf("Pacote corrompido\n");
            }

        } while(n == -1 || isCorrupt(rcvpkt) || isWrongState(rcvpkt, state));

        printf("\nEstado trocado\n");
        state = (state+1)%2;
        sleep(3);
    }
    
    close(sockfd); 
    return 0; 
}