#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "yen.h"

bool readUint32(int fd, uint32_t* value) {
    uint32_t networkValue;
    char* buf = reinterpret_cast<char*>(&networkValue);
    int count = 0;
    size_t total = 0;

    while (total < sizeof(uint32_t) && (count = read(fd, buf + total, sizeof(uint32_t) - total))) {
        total += count;
    }

    if (count < 0) {
        perror("read");
    }

    *value = ntohl(networkValue);
    return total == sizeof(uint32_t);
}

bool readGraph(int fd, std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& graph) {
    uint32_t n;

    if (!readUint32(fd, &n)) {
        return false;
    }

    graph = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>(n);

    for (size_t i = 0; i < n; ++i) {
        uint32_t deg;

        if (!readUint32(fd, &deg)) {
            return false;
        }

        graph[i] = std::vector<std::pair<uint32_t, uint32_t>>(deg);

        for (size_t j = 0; j < deg; ++j) {
            uint32_t u, w;

            if (!readUint32(fd, &u) || !readUint32(fd, &w)) {
                return false;
            }

            graph[i][j] = { u, w };
        }
    }

    return true;
}

template <typename T>
ssize_t send32(const int fd, const T n) {
    static_assert(sizeof(T) == 4, "Value must be 32-bit.");
    uint32_t nToSend = htonl(static_cast<uint32_t>(n));
    return write(fd, &nToSend, sizeof(uint32_t));
}

bool sendPath(const int fd, const path& p) {
    if (send32<uint32_t>(fd, p.size()) < 0) {
        perror("write");
        return false;
    }

    for (size_t i = 0; i < p.size(); ++i) {
        if (send32<uint32_t>(fd, p[i]) < 0) {
            return false;
        }
    }

    return true;
}

bool sendPaths(const int fd, const std::vector<path>& paths) {
    if (send32<uint32_t>(fd, paths.size()) < 0) {
        perror("write");
        return false;
    }

    for (size_t i = 0; i < paths.size(); ++i) {
        if (!sendPath(fd, paths[i])) {
            return false;
        }
    }

    return true;
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

    std::vector<std::vector<edge>> graph;
    uint32_t start, end, k;

    if (!readGraph(client_fd, graph) || !readUint32(client_fd, &start) || !readUint32(client_fd, &end) || !readUint32(client_fd, &k)) {
        std::cout << "An error occured.\n";
        close(client_fd);
        close(s);
        return -1;
    }

    if (start >= graph.size() || end >= graph.size()) {
        if (send32<int32_t>(client_fd, -1) < 0 || write(client_fd, "Start or end is not a valid vertex!\n", 36) < 0) {
            perror("write");
            close(client_fd);
            close(s);
            return -1;
        }
    }

    if (k == 0) {
        if (send32<int32_t>(client_fd, -1) < 0 || write(client_fd, "K must be at least 1!\n", 22) < 0) {
            perror("write");
            close(client_fd);
            close(s);
            return -1;
        }
    }

    std::vector<path> paths = yen(graph, start, end, k);
    if (!sendPaths(client_fd, paths)) {
        std::cout << "An error occured.\n";
        close(client_fd);
        close(s);
        return -1;
    }

    close(client_fd);
    close(s);
    return 0;
}