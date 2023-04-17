#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8888
#define BUFFER_SIZE 1024

int main() {
    int sockfd, n;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in servaddr, cliaddr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    FILE *fp = fopen("received_file.txt", "wb");
    while (1) {
        socklen_t len = sizeof(cliaddr);
        n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);
        if (n < 0) {
            printf("Error receiving data\n");
            exit(1);
        }
        fwrite(buffer, 1, n, fp);
        if (n < BUFFER_SIZE) break; 
    }
    fclose(fp);

    close(sockfd);

    printf("File received successfully!\n");

    return 0;
}

