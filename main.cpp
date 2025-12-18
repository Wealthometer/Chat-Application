#include <iostream>
#include <string>
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

// ==================== SERVER CODE ====================
mutex clients_mutex;
map<int, string> clients;

void handle_client(int client_socket, string client_name) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            lock_guard<mutex> guard(clients_mutex);
            cout << "[SERVER] " << client_name << " has disconnected." << endl;
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        string message = client_name + ": " + string(buffer);
        cout << "[CHAT] " << message << endl;

        lock_guard<mutex> guard(clients_mutex);
        for (auto& client : clients) {
            if (client.first != client_socket) {
                send(client.first, message.c_str(), message.length(), 0);
            }
        }
    }
}

void run_server() {
    int server_socket;
    struct sockaddr_in server_addr;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error binding socket" << endl;
        return;
    }

    if (listen(server_socket, 5) < 0) {
        cerr << "Error listening" << endl;
        return;
    }

    cout << "===== CHAT SERVER STARTED =====" << endl;
    cout << "Listening on port 8080..." << endl;
    cout << "Press Ctrl+C to stop server" << endl << endl;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket,
                                  (struct sockaddr*)&client_addr,
                                  &client_len);

        if (client_socket < 0) {
            cerr << "Error accepting connection" << endl;
            continue;
        }

        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        recv(client_socket, buffer, sizeof(buffer), 0);
        string client_name = string(buffer);

        {
            lock_guard<mutex> guard(clients_mutex);
            clients[client_socket] = client_name;

            string welcome_msg = "Welcome " + client_name + " to the chat!";
            send(client_socket, welcome_msg.c_str(), welcome_msg.length(), 0);

            string join_msg = client_name + " has joined the chat!";
            cout << "[SERVER] " << join_msg << endl;
            for (auto& client : clients) {
                if (client.first != client_socket) {
                    send(client.first, join_msg.c_str(), join_msg.length(), 0);
                }
            }
        }

        thread client_thread(handle_client, client_socket, client_name);
        client_thread.detach();
    }

    close(server_socket);
}

// ==================== CLIENT CODE ====================
bool running = true;

void receive_messages(int socket) {
    char buffer[1024];

    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(socket, buffer, sizeof(buffer), 0);

        if (bytes_received <= 0) {
            cout << "\n[CLIENT] Disconnected from server" << endl;
            running = false;
            break;
        }

        cout << "\r" << string(buffer) << endl;
        cout << "You: " << flush;
    }
}

void run_client(string server_ip = "127.0.0.1") {
    int client_socket;
    struct sockaddr_in server_addr;
    string name;

    cout << "===== CHAT CLIENT =====" << endl;
    cout << "Enter your name: ";
    getline(cin, name);

    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        cerr << "Error creating socket" << endl;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    cout << "Connecting to server at " << server_ip << ":8080..." << endl;

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Error connecting to server" << endl;
        return;
    }

    send(client_socket, name.c_str(), name.length(), 0);

    cout << "Connected! Type your messages below." << endl;
    cout << "Commands: '/exit' to quit, '/users' to see online users" << endl;
    cout << "=====================================" << endl;

    thread receive_thread(receive_messages, client_socket);

    string message;
    while (running) {
        cout << "You: ";
        getline(cin, message);

        if (message == "/exit") {
            running = false;
            break;
        }

        if (message == "/users") {
            cout << "[CLIENT] Listing online users..." << endl;
            // You could implement this feature
            continue;
        }

        if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
            cerr << "Error sending message" << endl;
            break;
        }
    }

    receive_thread.join();
    close(client_socket);

    cout << "Disconnected. Goodbye!" << endl;
}

// ==================== MAIN FUNCTION ====================
int main(int argc, char* argv[]) {
    cout << "===== CONSOLE CHAT APPLICATION =====" << endl;

    if (argc > 1) {
        string mode = argv[1];

        if (mode == "server") {
            run_server();
        }
        else if (mode == "client") {
            if (argc > 2) {
                run_client(argv[2]);  // With custom IP
            } else {
                run_client();  // Default to localhost
            }
        }
        else {
            cout << "Usage:" << endl;
            cout << "  ./chat server               - Start as server" << endl;
            cout << "  ./chat client               - Start as client (localhost)" << endl;
            cout << "  ./chat client <ip_address>  - Start as client with custom IP" << endl;
        }
    }
    else {
        // Interactive mode
        cout << "Select mode:" << endl;
        cout << "1. Start Server" << endl;
        cout << "2. Start Client" << endl;
        cout << "3. Exit" << endl;
        cout << "Choice: ";

        int choice;
        cin >> choice;
        cin.ignore();  // Clear newline

        switch (choice) {
            case 1:
                run_server();
                break;
            case 2:
                cout << "Enter server IP (default: 127.0.0.1): ";
                string ip;
                getline(cin, ip);
                if (ip.empty()) {
                    run_client();
                } else {
                    run_client(ip);
                }
                break;
            case 3:
                cout << "Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice!" << endl;
        }
    }

    return 0;
}
