#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CHUNK_SIZE 1024

using namespace std;

void handle_client(int client_socket, struct sockaddr_in client_address);

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [port]" << endl;
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Failed to create socket." << endl;
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
        cerr << "Failed to bind to port " << port << "." << endl;
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) < 0) {
        cerr << "Failed to listen for client connections." << endl;
        exit(EXIT_FAILURE);
    }

    cout << "Server is listening on port " << port << "..." << endl;

    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_length);
        if (client_socket < 0) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        cout << "Client connected from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "." << endl;

        pid_t pid = fork();
        if (pid == -1) {
            cerr << "Failed to fork a new process for the client." << endl;
            close(client_socket);
            continue;
        }
        else if (pid == 0) { // child process
            close(server_socket);

            handle_client(client_socket, client_address);

            close(client_socket);
            exit(EXIT_SUCCESS);
        }
        else { // parent process
            close(client_socket);
        }
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket, struct sockaddr_in client_address)
{
    char buffer[CHUNK_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        cerr << "Failed to receive message from client." << endl;
        return;
    }

    string client_message = string(buffer, bytes_received);
    if (client_message.substr(0, 7) != "HI THERE" || client_message[7] != '\n') {
        cerr << "Invalid client request." << endl;
        return;
    }

    DIR* dir = opendir(".");
    if (dir == NULL) {
        cerr << "Failed to open directory." << endl;
        string error_message = "ERROR Failed to open directory\n";
        send(client_socket, error_message.c_str(), error_message.length(), 0);
        return;
    }

    int num_files = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            num_files++;
        }
    }

    if (num_files == 0) {
        string error_message = "ERROR No files to download\n";
        send(client_socket, error_message.c_str(), error_message.length(), 0);
        closedir(dir);
        return;
    }

    string file_list = "OK " + to_string(num_files) + "\r\n";
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            file_list += entry->d_name;
            file_list += "\r\n";
        }
    }
    file_list += "\r\n";

    closedir(dir);

    send(client_socket, file_list.c_str(), file_list.length(), 0);

    bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received < 0) {
        cerr << "Failed to receive message from client." << endl;
        return;
    }

    string file_name = string(buffer, bytes_received);
    file_name.pop_back(); // remove the trailing \n character

    int file_descriptor = open(file_name.c_str(), O_RDONLY);
    if (file_descriptor < 0) {
        string error_message = "ERROR File not found\n";
        send(client_socket, error_message.c_str(), error_message.length(), 0);
        return;
    }
    
    struct stat file_info;
    if (fstat(file_descriptor, &file_info) < 0) {
        cerr << "Failed to get file size." << endl;
        close(file_descriptor);
        return;
    }

    string response_message = "OK " + to_string(file_info.st_size) + "\r\n";
    send(client_socket, response_message.c_str(), response_message.length(), 0);

    off_t offset = 0;
    ssize_t bytes_sent = 0;
    while ((bytes_sent = sendfile(client_socket, file_descriptor, &offset, CHUNK_SIZE)) > 0) {}

    close(file_descriptor);
}

