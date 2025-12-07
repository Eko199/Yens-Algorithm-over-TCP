#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in addr = { AF_INET, htons(4095), 0 };

    if (bind(s, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(s, 20) < 0) {
        perror("listen");
        return -1;
    }

    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    int client_fd = accept(s, (struct sockaddr*) &clientAddr, &clientAddrLen);

    if (client_fd < 0) {
        perror("accept");
        return -1;
    }

    char buffer[1024];
    ssize_t count;

    while (count = read(client_fd, buffer, sizeof(buffer))) {
        write(1, buffer, count);
    }

    close(client_fd);
    close(s);
    return 0;
}