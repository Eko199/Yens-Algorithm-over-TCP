#include <bit>
#include <chrono>
#include <cstdio>
#include <errno.h>
#include <functional>
#include <iostream>
#include <netinet/in.h>
#include <cstring>
#include <csignal>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include "threadpool.h"
#include "yen.h"

#define MAX_USERS 4

const uint32_t MAX_THREADS = std::max(std::thread::hardware_concurrency() / MAX_USERS, 1u);
bool running = true;

void interruptHandler(int signum) {
    running = false;
    std::cout << "Closing the server...\n";
}

bool readUint32(const int fd, uint32_t* value) {
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

bool readGraph(const int fd, std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& graph) {
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
ssize_t send32(const int fd, const T value) {
    static_assert(sizeof(T) == 4, "Value must be 32-bit.");
    uint32_t nToSend = htonl(std::bit_cast<uint32_t>(value));
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

void serveClient(const int clientFd) {
    std::vector<std::vector<edge>> graph;
    uint32_t start, end, k, threads;

    if (!readGraph(clientFd, graph) || !readUint32(clientFd, &start) || !readUint32(clientFd, &end) 
        || !readUint32(clientFd, &k) || !readUint32(clientFd, &threads)) {
        std::cout << "An error occured.\n";
        close(clientFd);
        return;
    }

    bool error = false;
    const char* message;

    if (start >= graph.size() || end >= graph.size()) {
        error = true;
        message = "Start or end is not a valid vertex!\n";
    }

    if (!error && k == 0) {
        error = true;
        message = "K must be at least 1!\n";
    }

    if (!error && (threads == 0 || threads > MAX_THREADS)) {
        error = true;
        message = "Invalid thread count!\n";
    }

    if (error && (send32<int32_t>(clientFd, -1) < 0 || write(clientFd, message, strlen(message)) < 0)) {
        perror("write");
        close(clientFd);
        return;
    }

    const auto startTime = std::chrono::high_resolution_clock::now();
    std::vector<path> paths = yen(graph, start, end, k, threads);
    const auto endTime = std::chrono::high_resolution_clock::now();

    if (!sendPaths(clientFd, paths)) {
        std::cout << "An error occured.\n";
        close(clientFd);
        return;
    }

    const std::chrono::duration<float, std::milli> ms = endTime - startTime;
    if (send32<float>(clientFd, ms.count()) < 0) {
        perror("write");
        close(clientFd);
        return;
    }

    close(clientFd);
}

int main() {
    if (signal(SIGINT, interruptHandler) == SIG_ERR || signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return -1;
    }
    
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0) {
        perror("socket");
        return -1;
    }

    int reuse = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void*) &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(s);
        return -1;
    }

    struct sockaddr_in addr = { AF_INET, htons(4095), 0 };

    if (bind(s, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        close(s);
        return -1;
    }

    if (listen(s, MAX_USERS) < 0) {
        perror("listen");
        close(s);
        return -1;
    }

    Threadpool tpool(MAX_USERS);

    fd_set readFds, writeFds;
    FD_ZERO(&readFds);
    FD_ZERO(&writeFds);
    FD_SET(s, &readFds);
    int maxFd = s;

    while (running) {
        fd_set readFdsReady = readFds;
        fd_set writeFdsReady = writeFds;
        int cnt = select(maxFd + 1, &readFdsReady, &writeFdsReady, nullptr, nullptr);

        if (cnt < 0) {
            if (errno == EINTR) {
                break;
            }

            perror("select");
            close(s);
            return -1;
        }

        if (cnt == 0) {
            continue;
        }

        int oldMaxFd = maxFd;
        maxFd = s;
        
        for (int i = 0; i <= oldMaxFd; ++i) {
            if (FD_ISSET(i, &readFdsReady)) {
                if (i != s) {
                    FD_CLR(i, &readFds);
                    tpool.enqueue(std::bind(serveClient, i));
                    continue;
                }

                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int clientFd = accept(s, (struct sockaddr*) &clientAddr, &clientAddrLen);

                if (clientFd < 0) {
                    perror("accept");
                    continue;
                }

                FD_SET(clientFd, &writeFds);
                maxFd = std::max(maxFd, clientFd);
            } else if (FD_ISSET(i, &readFds)) {
                maxFd = std::max(maxFd, i);
            }

            if (FD_ISSET(i, &writeFdsReady)) {
                send32<uint32_t>(i, MAX_THREADS);
                FD_CLR(i, &writeFds);
                FD_SET(i, &readFds);
                maxFd = std::max(maxFd, i);
            }
        }
    }

    close(s);
    return 0;
}