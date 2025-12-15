#include <bit>
#include <chrono>
#include <errno.h>
#include <iostream>
#include <cstring>
#include <csignal>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "io.hpp"
#include "threadpool.hpp"
#include "yen.hpp"

#define MAX_USERS 4

const uint32_t MAX_THREADS = std::max(std::thread::hardware_concurrency() / MAX_USERS, 1u);
bool running = true;

void interruptHandler(int signum) {
    running = false;
    std::cout << "Closing the server...\n";
}

void serveClient(const int clientFd) {
    std::vector<std::vector<edge>> graph;
    uint32_t start, end, k, threads;

    if (!readGraph(clientFd, graph) || !read32<uint32_t>(clientFd, &start) || !read32<uint32_t>(clientFd, &end) 
        || !read32<uint32_t>(clientFd, &k) || !read32<uint32_t>(clientFd, &threads)) {
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
                FD_CLR(i, &writeFds);

                if (send32<uint32_t>(i, MAX_THREADS) < 0) {
                    perror("write");
                    close(i);
                    continue;
                }
                
                FD_SET(i, &readFds);
                maxFd = std::max(maxFd, i);
            }
        }
    }

    close(s);
    return 0;
}