// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
    
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
	printf("%ld\n",strlen(msg));
    for(int i=0;i<sizeof(msg);i++){
		if(msg[i] == 0) break;
        // printf("%c // %d    ",msg[i],msg[i]);
        // printf("%c ",msg[i]);
        printf("%d ",msg[i]);
    }
	printf("\n");
}
int get_checksum(char *msg){
    int checksum, sum=0, i;
    for(i=0;i<sizeof(msg);i++){
		if(msg[i] == 0) break;
        sum+=msg[i];
    }
    checksum=~sum;
    return checksum;
}

void sender_make_pkt(char *pkt, char *msg, int state){
    union CkSum checksum;
    char aux[MAXLINE] = {0};
    memset(aux,0,sizeof(aux));


    sprintf(aux,"%d",state);           // Add state 
    strcat(aux, msg);                               // Add msg

    checksum.dword = get_checksum(aux);
    printf("Soma: %d ",checksum.dword);
    printf("\n%d %d %d %d\n",
            checksum.byte0,
            checksum.byte1,
            checksum.byte2,
            checksum.byte3
            ); 
    sprintf(pkt, "%c%c%c%c%d%s",
            checksum.byte0,
            checksum.byte1,
            checksum.byte2,
            checksum.byte3,
            state,msg
            ); 
    printpacket(pkt);
    printf("\n\n");

}

/////////////////////////////////////////////////

int main(int argc, char **argv) { 
    int sockfd; 
    char rcvpkt[MAXLINE];  
    struct sockaddr_in servaddr; 
    struct timeval tout;
    
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    tout.tv_sec=3;
    tout.tv_usec=0;
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
    // while(1){
        sprintf(msg, "Samba enredo"); // Adicionar o valor state e checksum
        sender_make_pkt(sndpkt,msg,state);
        do{
            // rdt_send()
            sendto(sockfd, (const char *)sndpkt, strlen(sndpkt), 
                MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                    sizeof(servaddr)); 
            printf("\n%s sent.\n",msg); 
            //
        
            // rdt_rcv()
            n = recvfrom(sockfd, (char *)rcvpkt, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 
            //
        // while(n == -1 || (corrupt(rcvpkt) || isACK(rcvpkt, (state+1)%2))
        } while(n == -1); // rdt_rcv(rcvpkt) && (corrupt(rcvplt) || isACK(rcvpkt, (state+1)%2))
        
        state = (state+1)%2;
    // }
    
    close(sockfd); 
    return 0; 
}