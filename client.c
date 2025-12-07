#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr = { AF_INET, htons(4095), 0 };

    if (connect(s, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return -1;
    }

    write(s, "Hello, Server!", 14);

    close(s);
    return 0;
}