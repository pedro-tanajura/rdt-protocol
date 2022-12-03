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
    
// Driver code 
int main(int argc, char **argv) { 
    int sockfd; 
    char rcvpkt[MAXLINE];  
    struct sockaddr_in servaddr; 
    struct timeval tout;
    
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
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
    
    while(1){
        // make_pkt(state, data, checksum)
        char *sndpkt;
        sprintf(sndpkt, "%d", state); // Adicionar o valor state e checksum

        do{
            // rdt_send()
            sendto(sockfd, (const char *)sndpkt, strlen(sndpkt), 
                MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                    sizeof(servaddr)); 
            printf("Hello message sent.\n"); 
            //
        
            // rdt_rcv()
            n = recvfrom(sockfd, (char *)rcvpkt, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 
            //
        // while(n == -1 || (corrupt(rcvpkt) || isACK(rcvpkt, (state+1)%2))
        } while(n == -1); // rdt_rcv(rcvpkt) && (corrupt(rcvplt) || isACK(rcvpkt, (state+1)%2))
        
        state = (state+1)%2;
    }
    
    close(sockfd); 
    return 0; 
}