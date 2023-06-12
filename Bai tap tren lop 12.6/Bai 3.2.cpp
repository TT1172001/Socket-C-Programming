#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MESSAGE_SIZE 1024

using namespace std;

void handle_client(int client_socket, queue<int>& client_queue, mutex& queue_mutex, condition_variable& queue_cv);

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

    queue<int> client_queue; // queue to store client sockets waiting for pairing
    mutex queue_mutex; // mutex to protect the client queue
    condition_variable queue_cv; // condition variable to signal when there are 2 clients waiting in the queue

    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);

        int client_socket = accept(server_socket, (struct sockaddr*) &client_address, &client_address_length);
        if (client_socket < 0) {
            cerr << "Failed to accept client connection." << endl;
            continue;
        }

        cout << "Client connected from " << inet_ntoa(client_address.sin_addr) << ":" << ntohs(client_address.sin_port) << "." << endl;

        thread client_thread(handle_client, client_socket, ref(client_queue), ref(queue_mutex), ref(queue_cv)); // create a new thread to handle the client request
        client_thread.detach(); // detach the thread to run independently
    }

    close(server_socket);
    return 0;
}

void handle_client(int client_socket, queue<int>& client_queue, mutex& queue_mutex, condition_variable& queue_cv)
{
    char buffer[MESSAGE_SIZE];
    int bytes_received;

    unique_lock<mutex> queue_lock(queue_mutex);
    client_queue.push(client_socket);

    if (client_queue.size() < 2) {
        cout << "Waiting for another client to connect..." << endl;
        queue_cv.wait(queue_lock, [&]{ return client_queue.size() == 2; }); // wait for another client to connect
    }

    int client1_socket = client_queue.front();
    client_queue.pop();
    int client2_socket = client_queue.front();
    client_queue.pop();

    cout << "Clients " << client1_socket << " and " << client2_socket << " are paired." << endl;

    queue_lock.unlock();

    while (true) {
        bytes_received = recv(client_socket, buffer, MESSAGE_SIZE, 0);
        if (bytes_received <= 0) {
            cout << "Client " << client_socket << " disconnected." << endl;
            shutdown(client1_socket, SHUT_RDWR);
            shutdown(client2_socket, SHUT_RDWR);
            break;
        }

        buffer[bytes_received] = '\0';
        cout << "Client " << client_socket << ": " << buffer;

        if (client_socket == client1_socket) { // forward message from client 1 to client 2
            send(client2_socket, buffer, bytes_received, 0);
        }
        else { // forward message from client 2 to client 1
            send(client1_socket, buffer, bytes_received, 0);
        }
    }

    close(client_socket);
}

