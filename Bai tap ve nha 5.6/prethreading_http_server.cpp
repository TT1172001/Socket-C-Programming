#include <iostream>
#include <thread>
#include <vector>
#include <queue>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

const int SERVER_PORT = 8080;
const int MAX_CONNECTIONS = 10;
const int BUFFER_SIZE = 1024;
const int THREAD_POOL_SIZE = 4;

struct Client {
    int socket;
    sockaddr_in address;
};

std::queue<Client> clientQueue;
std::mutex clientQueueMutex;
std::condition_variable clientQueueCondVar;
std::vector<std::thread> threadPool;

void handleClient(Client client) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = recv(client.socket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0) {
        close(client.socket);
        std::cout << "Client disconnected\n";
        return;
    }
    std::string request(buffer, bytesReceived);
    std::cout << "Received: " << request << std::endl;
    // Parse HTTP request
    std::string method;
    std::string path;
    std::string version;
    size_t pos1 = request.find(' ');
    if (pos1 != std::string::npos) {
        method = request.substr(0, pos1);
        size_t pos2 = request.find(' ', pos1 + 1);
        if (pos2 != std::string::npos) {
            path = request.substr(pos1 + 1, pos2 - pos1 - 1);
            size_t pos3 = request.find("\r\n", pos2 + 1);
            if (pos3 != std::string::npos) {
                version = request.substr(pos2 + 1, pos3 - pos2 - 1);
            } else {
                version = "HTTP/1.0";
            }
        }
    }
    std::cout << "Method: " << method << std::endl;
    std::cout << "Path: " << path << std::endl;
    std::cout << "Version: " << version << std::endl;
    // Generate HTTP response
    std::string response = version + " 200 OK\r\n";
    response += "Content-Type: text/html\r\n";
    response += "Connection: close\r\n";
    response += "\r\n";
    response += "<html><body><h1>Hello, world!</h1></body></html>";
    send(client.socket, response.c_str(), response.size(), 0);
    close(client.socket);
}

void workerThread() {
    while (true) {
        Client client;
        {
            std::unique_lock<std::mutex> lock(clientQueueMutex);
            clientQueueCondVar.wait(lock, []{ return !clientQueue.empty(); });
            client = clientQueue.front();
            clientQueue.pop();
        }
        handleClient(client);
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
    // Start worker threads
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        threadPool.emplace_back(workerThread);
    }

    while (true) {
        sockaddr_in clientAddress {};
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket < 0) {
            std::cerr << "Accept failed\n";
            break;
        }
        Client client = {clientSocket, clientAddress};
        {
            std::lock_guard<std::mutex> lock(clientQueueMutex);
            clientQueue.push(client);
        }
        clientQueueCondVar.notify_one();
    }

    for (auto& thread : threadPool) {
        thread.join();
    }
    close(serverSocket);
    return 0;
}

