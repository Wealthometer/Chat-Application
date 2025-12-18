#include <iostream>
#include <cstring>
#include <thread>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

bool running = true;

// Function to receive messages from server
void receive_messages(int socket) {
    char buffer[1024];

    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            cout << "\nDisconnected from server" << endl;
            running = false;
            break;
        }

        cout << "\n" << string(buffer) << endl;
        cout << "You: " << flush;
    }
}

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    string name;

    cout << "Enter your name: ";
    getline(cin, name);

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "Error creating socket" << endl;
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error connecting to server" << endl;
        return 1;
    }

    // Send name to server
    send(client_socket, name.c_str(), name.length(), 0);

    cout << "Connected to server. Type your messages below." << endl;
    cout << "Type 'exit' to quit." << endl;

    // Start receive thread
    thread receive_thread(receive_messages, client_socket);

    // Send messages
    string message;
    while (running) {
        cout << "You: ";
        getline(cin, message);

        if (message == "exit") {
            running = false;
            break;
        }

        if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
            cerr << "Error sending message" << endl;
            break;
        }
    }

    // Cleanup
    receive_thread.join();
    close(client_socket);

    cout << "Disconnected. Goodbye!" << endl;
    return 0;
}
