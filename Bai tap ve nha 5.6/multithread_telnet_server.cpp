#include <iostream>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

const int SERVER_PORT = 23;
const int MAX_CONNECTIONS = 10;
const int BUFFER_SIZE = 256;

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    int bytesReceived;
    // Send initial Telnet handshake to client
    send(clientSocket, "\xFF\xFD\x01\xFF\xFD\x03\xFF\xFB\x03", 9, 0);
    while (true) {
        bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            close(clientSocket);
            std::cout << "Client disconnected\n";
            break;
        }
        // Handle Telnet commands from client
        for (int i = 0; i < bytesReceived; i++) {
            if (buffer[i] == '\xFF' && i + 1 < bytesReceived) {
                if (buffer[i + 1] == '\xFB') { // WILL command
                    buffer[i + 1] = '\xFD'; // DON'T
                    send(clientSocket, buffer + i, 2, 0);
                } else if (buffer[i + 1] == '\xFD') { // DO command
                    buffer[i + 1] = '\xFB'; // WILL
                    send(clientSocket, buffer + i, 2, 0);
                }
            }
        }
        buffer[bytesReceived] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        // Echo message back to client
        send(clientSocket, buffer, bytesReceived, 0);
    }
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddress {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    if (listen(serverSocket, MAX_CONNECTIONS) < 0) {
        std::cerr << "Listen failed\n";
        return 1;
    }

    std::cout << "Server started\n";

    while (true) {
        sockaddr_in clientAddress {};
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket < 0) {
            std::cerr << "Accept failed\n";
            break;
        }
        std::cout << "Client connected\n";
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}

