#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_MSG_LENGTH 1024

typedef struct {
    char mssv[10];
    char fullname[100];
    char dob[11];
    float gpa;
} Student;

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server address> <port>\n", argv[0]);
        exit(1);
    }
    
    char *server_address = argv[1];
    int port = atoi(argv[2]);
    
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }
    
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(server_address);
    servaddr.sin_port = htons(port);
    
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }
    
    Student student;
    
    printf("Enter MSSV: ");
    fgets(student.mssv, sizeof(student.mssv), stdin);
    student.mssv[strlen(student.mssv) - 1] = '\0';  // remove newline character
    
    printf("Enter full name: ");
    fgets(student.fullname, sizeof(student.fullname), stdin);
    student.fullname[strlen(student.fullname) - 1] = '\0';  // remove newline character
    
    printf("Enter date of birth (DD/MM/YYYY): ");
    fgets(student.dob, sizeof(student.dob), stdin);
    student.dob[strlen(student.dob) - 1] = '\0';  // remove newline character
    
    printf("Enter GPA: ");
    scanf("%f", &student.gpa);
    
    char message[MAX_MSG_LENGTH];
    snprintf(message, MAX_MSG_LENGTH, "%s,%s,%s,%.2f", student.mssv, student.fullname, student.dob, student.gpa);
    
    if (write(sockfd, message, strlen(message)) < 0) {
        perror("write");
        exit(1);
    }
    
    close(sockfd);
    
    return 0;
}

