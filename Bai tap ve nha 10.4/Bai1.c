#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#define MAX_MSG_LENGTH 1024

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP address> <port>\n", argv[0]);
        exit(1);
    }
    
    char *ip_address = argv[1];
    int port = atoi(argv[2]);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid IP address\n");
        exit(1);
    }
    
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    char buffer[MAX_MSG_LENGTH];
    while (fgets(buffer, MAX_MSG_LENGTH, stdin)) {
        if (write(sockfd, buffer, strlen(buffer)) < 0) {
            perror("write");
            exit(1);
        }
    }
    
    close(sockfd);
    
    return 0;
}

