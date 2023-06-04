#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <cstdlib>

#define MAX_CONNECTIONS 100

void handle_client(int sockfd) {
    char buffer[1024] = {0};
    std::string response = "Welcome to Telnet Server\n";
    send(sockfd, response.c_str(), response.length(), 0);

    while (true) {
        int valread = read(sockfd, buffer, 1024);
        if (valread <= 0) {
            std::cerr << "Client disconnected" << std::endl;
            break;
        }
        std::string command(buffer);
        if (command == "exit\n") {
            std::cerr << "Client exited" << std::endl;
            break;
        }
        std::cout << "Received command from client: " << command << std::endl;

        // Execute command and send output to client
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            std::cerr << "Failed to execute command" << std::endl;
            continue;
        }
        char result[1024] = {0};
        while (fgets(result, sizeof(result), pipe)) {
            // Send output to client
            send(sockfd, result, strlen(result), 0);
            std::cout << result << std::endl;
        }
        pclose(pipe);
    }

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

    while (true) {
        new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Failed to accept connection" << std::endl;
            return 1;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "Failed to fork process" << std::endl;
            return 1;
        }
        if (pid == 0) {
            // Child process
            handle_client(new_socket);
            exit(0);
        }
        else {
            close(new_socket);
        }
    }

    return 0;
}

