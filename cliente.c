#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include <sys/time.h>
#include <stdbool.h>

#define PORT    5000 
#define MAXLINE 1000
    
int oncethru = 0;

struct pacote{
    int id;
    bool ack;
    char msg[MAXLINE];
};

int main(int argc, char **argv) { 
    int sockfd; 
    char buffer[MAXLINE]; 
    struct sockaddr_in     servaddr; 
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
        
    int n, len; 

    // Preencher pct
    struct pacote pct;
    strcpy(pct.msg, argv[3]);
    pct.id = 0;
    pct.ack = false;

    // rdt_send()
    do{ 
        sendto(sockfd, (const struct pacote *)&pct, sizeof(struct pacote), 
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 

        printf("Mensagem enviada.\n"); 
        
        n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                    MSG_WAITALL, (struct sockaddr *) &servaddr, 
                    &len); 

        if(n > 0){
            printf("ACK");
            // ACK
            //sendto(sockfd, (const char *)msg, strlen(msg), 
                //MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
                //sizeof(servaddr)); 
        }
    } while(n == 0);
    // Cliente só sai do loop se o servidor fechar a conexão (n=0)

    buffer[n] = '\0'; 
    printf("Receiver: %s\n", buffer); 

    close(sockfd); 
    return 0; 
}