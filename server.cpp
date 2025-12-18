#include <iostream>
#include <cstring>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// Global variables
mutex clients_mutex;
map<int, string> clients;

// Function to handle client communication
void handle_client(int client_socket, string client_name) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            // Client disconnected
            lock_guard<mutex> guard(clients_mutex);
            cout << client_name << " has disconnected." << endl;
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        string message = client_name + ": " + string(buffer);
        cout << message << endl;

        // Broadcast message to all other clients
        lock_guard<mutex> guard(clients_mutex);
        for (auto& client : clients) {
            if (client.first != client_socket) {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }
}

int main() {
    int server_socket;
    struct sockaddr_in server_addr;

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error binding socket" << endl;
        return 1;
    }

    // Listen for connections
    if (listen(server_socket, 5) < 0) {
        cerr << "Error listening" << endl;
        return 1;
    }

    cout << "Server started on port 8080..." << endl;
    cout << "Waiting for connections..." << endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Accept new connection
        int client_socket = accept(server_socket,
                                  (struct sockaddr*)&client_addr,
                                  &client_len);

        if (client_socket < 0) {
            cerr << "Error accepting connection" << endl;
            continue;
        }

        // Get client name
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);
        string client_name = string(buffer);

        {
            lock_guard<mutex> guard(clients_mutex);
            clients[client_socket] = client_name;

            // Send welcome message
            string welcome_msg = "Welcome " + client_name + " to the chat!";
            send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);

            // Notify all clients about new user
            string join_msg = client_name + " has joined the chat!";
            cout << join_msg << endl;
            for (auto& client : clients) {
                if (client.first != client_socket) {
                    send(client.first, join_msg.c_str(), join_msg.length(), 0);
                }
            }
        }

        // Create thread for client
        thread client_thread(handle_client, client_socket, client_name);
        client_thread.detach();
    }

    close(server_socket);
    return 0;
}
