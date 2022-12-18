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
#include "rdt.h"

#define PORT 5000 

int main(int argc, char **argv) { 
    int sockfd;  
    struct timeConfig TConf;
    TConf.DevRTT = 0.05;
    TConf.EstRTT = 4;
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
        
    printf("%s\n", argv[1]);
    printf("%s\n", argv[2]);
    printf("%s\n", argv[3]);
    
    char msg[MAXLINE] = {0};
    int state = 0;

    sprintf(msg, argv[3]);
    while(1){
        rdt_snd(sockfd, msg, state, argv[2], argv[1],(void *)&TConf);
        printf("\nEstado trocado\n");
        state = (state+1)%2;
    }
    
    close(sockfd); 
    return 0; 
}