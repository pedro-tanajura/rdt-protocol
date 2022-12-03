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
    char buffer[MAXLINE]; 
    char *hello = argv[3]; 
    struct sockaddr_in     servaddr; 
    
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
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
        
    sendto(sockfd, (const char *)hello, strlen(hello), 
        MSG_CONFIRM, (const struct sockaddr *) &servaddr,  
            sizeof(servaddr)); 
    printf("Hello message sent.\n"); 
            
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,  
                MSG_WAITALL, (struct sockaddr *) &servaddr, 
                &len); 
    buffer[n] = '\0'; 
    printf("Server : %s\n", buffer); 
    
    close(sockfd); 
    return 0; 
}