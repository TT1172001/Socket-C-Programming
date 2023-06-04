#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctime>
#include <sstream>

#define MAX_CONNECTIONS 100

void handle_client(int sockfd) {
    char buffer[1024] = {0};
    int valread = read(sockfd, buffer, 1024);
    std::cout << "Received message from client: " << buffer << std::endl;
    std::string client_request(buffer);

    // Get current time
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);

    // Parse client request to get time format
    size_t pos = client_request.find(' ');
    std::string time_format = client_request.substr(pos+1);

    // Convert time to the requested format
    std::string time_string;
    if (time_format == "dd/mm/yyyy") {
        std::stringstream ss;
        ss << timeinfo->tm_mday << "/" << timeinfo->tm_mon + 1 << "/" << timeinfo->tm_year + 1900;
        time_string = ss.str();
    }
    else if (time_format == "dd/mm/yy") {
        std::stringstream ss;
        ss << timeinfo->tm_mday << "/" << timeinfo->tm_mon + 1 << "/" << (timeinfo->tm_year + 1900) % 100;
        time_string = ss.str();
    }
    else if (time_format == "mm/dd/yyyy") {
        std::stringstream ss;
        ss << timeinfo->tm_mon + 1 << "/" << timeinfo->tm_mday << "/" << timeinfo->tm_year + 1900;
        time_string = ss.str();
    }
    else if (time_format == "mm/dd/yy") {
        std::stringstream ss;
        ss << timeinfo->tm_mon + 1 << "/" << timeinfo->tm_mday << "/" << (timeinfo->tm_year + 1900) % 100;
        time_string = ss.str();
    }
    else {
        time_string = "Invalid time format";
    }

    // Send time to client
    send(sockfd, time_string.c_str(), time_string.length(), 0);
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

