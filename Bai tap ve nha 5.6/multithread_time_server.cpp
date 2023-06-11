#include <iostream>
#include <thread>
#include <chrono>
#include <sstream>
#include <ctime>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_client(int client_socket) {
    // Get current time
    std::time_t t = std::time(nullptr);
    std::tm time_info = *std::localtime(&t);

    // Convert time to string
    std::ostringstream oss;
    oss << std::put_time(&time_info, "%c");

    std::string time_str = oss.str();

    // Send time string to client
    write(client_socket, time_str.c_str(), time_str.length());

    // Close the client connection
    close(client_socket);
}

int main() {
    // Create socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket == -1) {
        std::cerr << "Error creating socket!" << std::endl;
        return 1;
    }

    // Bind socket to IP address and port
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(8080);

    if (bind(server_socket, (sockaddr*) &server_address, sizeof(server_address)) == -1) {
        std::cerr << "Error binding socket to port!" << std::endl;
        return 1;
    }

    // Listen for client connections
    if (listen(server_socket, SOMAXCONN) == -1) {
        std::cerr << "Error listening for connections!" << std::endl;
        return 1;
    }

    std::cout << "Server started." << std::endl;

    while (true) {
        // Accept client connection
        sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);

        int client_socket = accept(server_socket, (sockaddr*) &client_address, &client_address_size);

        if (client_socket == -1) {
            std::cerr << "Error accepting client connection!" << std::endl;
            continue;
        }

        std::cout << "Client connected: " << inet_ntoa(client_address.sin_addr) << std::endl;

        // Create thread to handle client connection
        std::thread handle_client_thread(handle_client, client_socket);
        handle_client_thread.detach();
    }

    // Close socket when finished
    close(server_socket);

    return 0;
}

