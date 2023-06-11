#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

const int SERVER_PORT = 12345;
const int MAX_CONNECTIONS = 10;
const int BUFFER_SIZE = 256;

std::vector<int> clientSockets;
std::mutex clientSocketsMutex;

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            close(clientSocket);
            std::cout << "Client disconnected\n";
            std::lock_guard<std::mutex> lock(clientSocketsMutex);
            clientSockets.erase(std::remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end());
            break;
        }
        buffer[bytesReceived] = '\0';
        std::cout << "Received: " << buffer << std::endl;
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        for (int i = 0; i < clientSockets.size(); i++) {
            if (clientSockets[i] != clientSocket) {
                send(clientSockets[i], buffer, bytesReceived + 1, 0);
            }
        }
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
        std::lock_guard<std::mutex> lock(clientSocketsMutex);
        clientSockets.push_back(clientSocket);
        std::thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}

