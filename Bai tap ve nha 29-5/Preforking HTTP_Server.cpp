#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_CONNECTIONS 100

void handle_client(int sockfd) {
    char buffer[1024] = {0};
    int valread = read(sockfd, buffer, 1024);
    std::cout << "Received message from client: " << buffer << std::endl;
    const char* hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
    send(sockfd, hello, strlen(hello), 0);
    std::cout << "Response sent to client" << std::endl;
    close(sockfd);
}

void handle_sigchld(int sig) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "Failed to set socket options" << std::endl;
        return 1;
    }

    // Bind socket to a specified IP address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        std::cerr << "Failed to listen for connections" << std::endl;
        return 1;
    }

    // Handle SIGCHLD signal to prevent zombie processes
    signal(SIGCHLD, handle_sigchld);

    for (int i = 0; i < 3; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Failed to fork process" << std::endl;
            return 1;
        }
        if (pid == 0) {
            // Child process
            while (true) {
                new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                if (new_socket < 0) {
                    std::cerr << "Failed to accept connection" << std::endl;
                    return 1;
                }
                handle_client(new_socket);
            }
        }
    }

    return 0;
}

