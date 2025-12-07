#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

int main() {
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = { AF_INET, htons(4095), 0 };

    if (connect(s, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }

    int32_t n;
    std::cout << "How many vertices does the graph have? (1-N): ";

    do {
        std::cin >> n;

        if (n <= 0) {
            std::cout << "Invalid input, must be positive! Try again: ";
        }
    } while (n <= 0);

    uint32_t un = htonl(n);
    write(s, &un, sizeof(uint32_t));

    close(s);
    return 0;
}