#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace std;

vector<int> clients;
mutex clients_mutex;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "syntax : echo-server <port> [-e[-b]]\n";
        cerr << "sample : echo-server 1234 -e -b\n";
        return 1;
    }

    int port = stoi(argv[1]);
    bool echo = false;
    bool broadcast = false;

    for (int i = 2; i < argc; i++) {
        string option = argv[i];
        if (option == "-e") {
            echo = true;
        } else if (option == "-b") {
            broadcast = true;
        }
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("socket");
        return 1;
    }

  

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, 10) == -1) {
        perror("listen");
        close(server_sock);
        return 1;
    }

    cout << "Server listening on port " << port << endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_len);

        if (client_sock == -1) {
            perror("accept");
            continue;
        }

        {
            lock_guard<mutex> lock(clients_mutex);
            clients.push_back(client_sock);
        }

        cout << "Client connected: " << client_sock << endl;

        thread([client_sock, echo, broadcast]() {
            char buffer[1024];

            while (true) {
                ssize_t received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
                if (received <= 0) {
                    break;
                }

                buffer[received] = '\0';
                string message(buffer, received);

                cout << "[client " << client_sock << "] " << message << endl;

                if (broadcast) {
                    lock_guard<mutex> lock(clients_mutex);
                    for (int sock : clients) {
                        send(sock, message.c_str(), message.size(), 0);
                    }
                } else if (echo) {
                    send(client_sock, message.c_str(), message.size(), 0);
                }
            }

            {
                lock_guard<mutex> lock(clients_mutex);
                clients.erase(std::remove(clients.begin(), clients.end(), client_sock), clients.end());
            }

            close(client_sock);
            cout << "Client disconnected: " << client_sock << endl;
        }).detach();
    }

    close(server_sock);
    return 0;
}
