#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "syntax : echo-client <ip> <port>\n";
        cerr << "sample : echo-client 192.168.10.2 1234\n";
        return 1;
    }

    const char* ip = argv[1];
    int port = stoi(argv[2]);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        cerr << "Invalid IP address\n";
        close(sock);
        return 1;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }

    bool running = true;

    
    thread recv_thread([&]() {
        char buffer[1024];

        while (running) {
            ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (received <= 0) {
                running = false;
                break;
            }

            buffer[received] = '\0';
            cout << "[server] " << buffer << endl;
        }
    });

    string message;
    while (running && getline(cin, message)) {
        if (message == "quit") {
            running = false;
            break;
        }

        if (send(sock, message.c_str(), message.size(), 0) == -1) {
            perror("send");
            running = false;
            break;
        }
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

    if (recv_thread.joinable()) {
        recv_thread.detach();
    }

    return 0;
}
