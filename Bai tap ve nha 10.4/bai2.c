#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_MSG_LENGTH 1024

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <port> <greeting file> <file to save client content>\n", argv[0]);
        exit(1);
    }
    
    int port = atoi(argv[1]);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }
    
    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }
    
    int greeting_file_fd = open(argv[2], O_RDONLY);
    if (greeting_file_fd < 0) {
        perror("open");
        exit(1);
    }
    
    char greeting[MAX_MSG_LENGTH];
    ssize_t greeting_len = read(greeting_file_fd, greeting, MAX_MSG_LENGTH);
    if (greeting_len < 0) {
        perror("read");
        exit(1);
    }
    
    int client_sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    
    while ((client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addrlen)) >= 0) {
        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        if (write(client_sockfd, greeting, greeting_len) < 0) {
            perror("write");
            exit(1);
        }
        
        char client_buffer[MAX_MSG_LENGTH];
        ssize_t client_buffer_len;
        
        int client_file_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (client_file_fd < 0) {
            perror("open");
            exit(1);
        }
        
        while ((client_buffer_len = read(client_sockfd, client_buffer, MAX_MSG_LENGTH)) > 0) {
            if (write(client_file_fd, client_buffer, client_buffer_len) < 0) {
                perror("write");
                exit(1);
            }
        }
        
        if (client_buffer_len < 0) {
            perror("read");
            exit(1);
        }
        
        close(client_file_fd);
        close(client_sockfd);
    }
    
    close(sockfd);
    
    return 0;
}

