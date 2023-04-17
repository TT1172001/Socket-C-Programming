#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_FILE_SIZE 100000

int main(int argc, char *argv[]) {
  int sockfd, n;
  struct sockaddr_in servaddr;
  char file_name[100], file_content[MAX_FILE_SIZE];
  char buffer[1024];
  ssize_t file_size;

  if (argc != 4) {
    printf("usage: %s <IP address> <port number> <file name>\n", argv[0]);
    exit(1);
  }

  const char* ip_addr = argv[1];
  int port = atoi(argv[2]);

  strncpy(file_name, argv[3], sizeof(file_name));

  int file_fd = open(file_name, O_RDONLY);

  if (file_fd < 0) {
    printf("Failed to open file: %s\n", file_name);
    exit(1);
  }

  file_size = read(file_fd, file_content, MAX_FILE_SIZE);

  if (file_size < 0) {
    printf("Failed to read file: %s\n", file_name);
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd < 0) {
    printf("Failed to create socket.\n");
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);
  inet_pton(AF_INET, ip_addr, &servaddr.sin_addr);
  
  while(1){

  n = sendto(sockfd, file_name, sizeof(file_name), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

  if (n < 0) {
    printf("Failed to send file name.\n");
    exit(1);
  }

  n = sendto(sockfd, file_content, file_size, 0, (struct sockaddr *)&servaddr, sizeof(servaddr));

  if (n < 0) {
    printf("Failed to send file content.\n");
    exit(1);
  }

  printf("File sent successfully.\n");
}

  close(sockfd);
  close(file_fd);

  return 0;
}

