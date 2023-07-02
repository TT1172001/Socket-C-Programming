#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <dirent.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 4096
#define DEFAULT_PORT 8080

// Function to return the MIME type of a file based on its extension
std::string getMimeType(const std::string& path) {
    std::map<std::string, std::string> mimeTypes = {
        {"txt", "text/plain"},
        {"c", "text/plain"},
        {"cpp", "text/plain"},
        {"jpg", "image/jpeg"},
        {"png", "image/png"},
        {"mp3", "audio/mpeg"}
    };

    std::string extension = path.substr(path.find_last_of(".") + 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    if (mimeTypes.find(extension) != mimeTypes.end()) {
        return mimeTypes[extension];
    }

    return "application/octet-stream"; // Default MIME type
}

// Function to return the contents of a directory in HTML format
std::string getDirectoryContents(const std::string& path) {
    std::stringstream ss;
    ss << "<html><head><title>Index of " << path << "</title></head><body>";
    ss << "<h1>Index of " << path << "</h1><hr><pre>";

    DIR* dir;
    struct dirent* entry;
    dir = opendir(path.c_str());

    if (dir) {
        while ((entry = readdir(dir)) != NULL) {
            std::string name = entry->d_name;
            std::string fullPath = path + "/" + name;

            if (entry->d_type == DT_DIR) {
                ss << "<b><a href=\"" << fullPath << "\">" << name << "/</a></b><br>";
            } else {
                ss << "<i><a href=\"" << fullPath << "\">" << name << "</a></i><br>";
            }
        }
        closedir(dir);
    }

    ss << "</pre><hr></body></html>";

    return ss.str();
}

// Function to handle client requests
void handleRequest(int clientSocket) {
    char buffer[BUFFER_SIZE] = {0};
    recv(clientSocket, buffer, BUFFER_SIZE, 0);

    std::string request(buffer);

    std::string path = "." + request.substr(request.find(" ") + 1, request.find(" ", request.find(" ") + 1) - request.find(" ") - 1);
    // Remove query string if present
    if (path.find("?") != std::string::npos) {
        path = path.substr(0, path.find("?"));
    }

    // Check if directory or file exists
    bool isDirectory = false;
    DIR* dir;
    if ((dir = opendir(path.c_str())) != NULL) {
        isDirectory = true;
        closedir(dir);
    }

    std::string response;

    if (isDirectory) {
        response = getDirectoryContents(path);
    } else {
        std::ifstream file(path, std::ios::in | std::ios::binary);
        if (!file) {
            response = "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        } else {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string fileContents = buffer.str();
            file.close();

            std::stringstream ss;
            ss << "HTTP/1.1 200 OK\r\nContent-Length: " << fileContents.length() << "\r\nContent-Type: " << getMimeType(path) << "\r\n\r\n" << fileContents;
            response = ss.str();
        }
    }

    send(clientSocket, response.c_str(), response.length(), 0);
    close(clientSocket);
}

int main() {
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t clientAddrSize;

    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket < 0) {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    // Prepare server address structure
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(DEFAULT_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(serverSocket, (struct sockaddr*)&serverAddr , sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket" << std::endl;
        return -1;
    }

    // Listen for connections
    listen(serverSocket, 3);
    std::cout << "Server started on port " << DEFAULT_PORT << std::endl;

    while (true) {
        clientAddrSize = sizeof(clientAddr);

        // Accept incoming connections
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection" << std::endl;
            return -1;
        }

        std::cout << "New client connected" << std::endl;

        // Handle client request in a separate thread
        handleRequest(clientSocket);
    }

    close(serverSocket);

    return 0;
}

