#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>

int main() {
    const int PORT = 5025;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    listen(server_fd, 1);
    std::cout << "SCPI server listening on port " << PORT << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            perror("accept");
            break;
        }
        while (true) {
            char buffer[1024] = {0};
            ssize_t n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (n <= 0) break;

            std::string cmd(buffer);
            cmd.erase(cmd.find_last_not_of("\r\n") + 1);

            std::string reply;
            if (cmd == "*IDN?")             reply = "HP,EMULATOR,0,1.0\n";
            else if (cmd == "*RST")         reply = "OK\n";
            else if (cmd.rfind(":MEAS?", 0) == 0) reply = "123.45\n";
            else                              reply = "ERR:UNKNOWN\n";

            send(client_fd, reply.c_str(), reply.size(), 0);
        }
        close(client_fd);
    }
    close(server_fd);
    return 0;
}
