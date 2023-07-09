#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

void error(const std::string& errorMessage) {
    std::cerr << "Error: " << errorMessage << std::endl;
    exit(1);
}

int createSocket() {
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        error("Failed to create socket");
    }
    return clientSocket;
}

void connectToServer(int clientSocket, const std::string& host, int port) {
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &(serverAddress.sin_addr)) <= 0) {
        error("Invalid address/Address not supported");
    }

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("Failed to connect to the server");
    }
}

std::string receiveResponse(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    std::string response;

    int bytesRead;
    while ((bytesRead = read(clientSocket, buffer, BUFFER_SIZE - 1)) > 0) {
        response += buffer;
        memset(buffer, 0, sizeof(buffer));
    }

    if (bytesRead < 0) {
        error("Failed to receive response from server");
    }

    return response;
}

void sendCommand(int clientSocket, const std::string& command) {
    if (write(clientSocket, command.c_str(), command.length()) < 0) {
        error("Failed to send command to server");
    }
}

void loginUser(int clientSocket, const std::string& username, const std::string& password) {
    sendCommand(clientSocket, "USER " + username + "\r\n");
    receiveResponse(clientSocket);

    sendCommand(clientSocket, "PASS " + password + "\r\n");
    receiveResponse(clientSocket);
}

void uploadFile(int clientSocket, const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        error("Failed to open file: " + filePath);
    }

    sendCommand(clientSocket, "PASV\r\n");
    std::string response = receiveResponse(clientSocket);

    // Parse the response to get the server's data port
    int dataPort = 0;
    std::stringstream ss(response);
    std::string line;
    while (getline(ss, line)) {
        size_t pos = line.find("227 Entering Passive Mode (");
        if (pos != std::string::npos) {
            int octet1, octet2, octet3, octet4, p1, p2;
            sscanf(line.c_str(), "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)",
                &octet1, &octet2, &octet3, &octet4, &p1, &p2);
            dataPort = p1 * 256 + p2;
            break;
        }
    }

    if (dataPort == 0) {
        error("Failed to parse server response for PASV command");
    }

    int dataSocket = createSocket();
    connectToServer(dataSocket, "localhost", dataPort);
    sendCommand(clientSocket, "STOR " + filePath + "\r\n");
    receiveResponse(clientSocket);

    char buffer[BUFFER_SIZE] = {0};
    while (file.read(buffer, sizeof(buffer))) {
        if (write(dataSocket, buffer, sizeof(buffer)) < 0) {
            error("Failed to send file content");
        }
        memset(buffer, 0, sizeof(buffer));
    }

    file.close();
    close(dataSocket);

    receiveResponse(clientSocket);
}

int main() {
    std::string serverHost = "127.0.0.1";
    int serverPort = 21;
    std::string username = "username";
    std::string password = "password";
    std::string filePath = "/path/to/file";

    int clientSocket = createSocket();
    connectToServer(clientSocket, serverHost, serverPort);

    receiveResponse(clientSocket);
    loginUser(clientSocket, username, password);
    uploadFile(clientSocket, filePath);

    sendCommand(clientSocket, "QUIT\r\n");
    receiveResponse(clientSocket);

    close(clientSocket);

    return 0;
}

