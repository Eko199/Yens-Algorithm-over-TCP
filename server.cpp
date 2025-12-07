#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

bool readUint32(int fd, uint32_t* value) {
    uint32_t networkValue;
    char* buf = reinterpret_cast<char*>(&networkValue);
    int count = 0;
    size_t total = 0;

    while (total < sizeof(uint32_t) && (count = read(fd, buf + total, sizeof(uint32_t) - total))) {
        total += count;
    }

    *value = ntohl(networkValue);
    return total == sizeof(uint32_t);
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { AF_INET, htons(4095), 0 };

    if (bind(s, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    if (listen(s, 20) < 0) {
        perror("listen");
        close(s);
        return -1;
    }

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client_fd = accept(s, (struct sockaddr*) &clientAddr, &clientAddrLen);

    if (client_fd < 0) {
        perror("accept");
        return -1;
    }

    uint32_t n;
    
    if (!readUint32(client_fd, &n)) {
        std::cout << "An error occured.\n";
        close(client_fd);
        close(s);
        return -1;
    }

    std::cout << n;

    close(client_fd);
    close(s);
    return 0;
}