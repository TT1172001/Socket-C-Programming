#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    char buffer[MAX_BUFFER_SIZE];
    char log_filename[MAX_BUFFER_SIZE];
    time_t current_time;
    struct tm *local_time;
    FILE *log_file;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s port_number log_file_name\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create a socket for the server
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(atoi(argv[1]));

    // Bind the server socket to the server address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding server socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 1) < 0) {
        perror("Error listening for incoming connections");
        exit(EXIT_FAILURE);
    }

    printf("Server started on port %s\n", argv[1]);

    // Open the log file for writing
    strcpy(log_filename, argv[2]);
    if ((log_file = fopen(log_filename, "a")) == NULL) {
        perror("Error opening log file");
        exit(EXIT_FAILURE);
    }

    // Wait for incoming connections
    while (1) {
        socklen_t client_address_length = sizeof(client_address);

        // Accept an incoming connection
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length)) < 0) {
            perror("Error accepting incoming connection");
            exit(EXIT_FAILURE);
        }

        // Get the current time
        time(&current_time);
        local_time = localtime(&current_time);

        // Receive data from the client
        memset(buffer, 0, sizeof(buffer));
        if (recv(client_socket, buffer, sizeof(buffer), 0) < 0) {
            perror("Error receiving data from client");
            exit(EXIT_FAILURE);
        }

        // Print the data to the screen and write it to the log file
        printf("%s %s %s", inet_ntoa(client_address.sin_addr), asctime(local_time), buffer);
        fprintf(log_file, "%s %s %s", inet_ntoa(client_address.sin_addr), asctime(local_time), buffer);

        // Close the client socket
        close(client_socket);
    }

    // Close the log file and server socket
    fclose(log_file);
    close(server_socket);

    return 0;
}

