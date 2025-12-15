#include <bit>
#include <climits>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include "io.hpp"

std::ostream cnull(nullptr);
std::ostream& prompt = isatty(STDIN_FILENO) ? std::cout : cnull;

int getIntInput(const int min = INT_MIN, const int max = INT_MAX) {
    int result;

    do {
        std::cin >> result;

        if (result < min || result > max) {
            if (max == INT_MAX) {
                std::cout << "Invalid input, must be at least " << min;
            } else if (min == INT_MAX) {
                std::cout << "Invalid input, must be at most " << max;
            } else {
                std::cout << "Invalid input, must be at between " << min << " and " << max;
            }

            std::cout << ". Try again: ";
        }
    } while (result < min || result > max);

    return result;
}

std::vector<std::vector<std::pair<unsigned, unsigned>>> getGraphInput() {
    prompt << "How many vertices does the graph have? ";
    unsigned n = static_cast<unsigned>(getIntInput(1));

    std::vector<std::vector<std::pair<unsigned, unsigned>>> graph(n);

    for (size_t i = 0; i < n; ++i) {
        prompt << "Enter number of edges from vertex " << i << ": ";
        unsigned deg = getIntInput(0);

        graph[i] = std::vector<std::pair<unsigned, unsigned>>(deg);

        for (size_t j = 0; j < deg; ++j) {
            prompt << j + 1 << ") vertex: ";
            unsigned u = getIntInput(0, n - 1);

            prompt << j + 1 << ") weight: ";
            unsigned w = getIntInput(0);

            graph[i][j] = { u, w };
        }
    }

    return graph;
}

void sendUint(const int fd, const unsigned n) {
    uint32_t nToSend = htonl(n);

    if (write(fd, &nToSend, sizeof(uint32_t)) < 0) {
        perror("write");
        close(fd);
        exit(-1);
    }
}

void sendGraph(const int fd, const std::vector<std::vector<std::pair<unsigned, unsigned>>>& graph) {
    sendUint(fd, graph.size());

    for (size_t i = 0; i < graph.size(); ++i) {
        sendUint(fd, graph[i].size());

        for (size_t j = 0; j < graph[i].size(); ++j) {
            sendUint(fd, graph[i][j].first);
            sendUint(fd, graph[i][j].second);
        }
    }
}

int main() {
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return -1;
    }

    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = { AF_INET, htons(4095), 0 };

    if (connect(s, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }

    std::vector<std::vector<std::pair<unsigned, unsigned>>> graph = getGraphInput();

    prompt << "Enter start vertex: ";
    unsigned start = getIntInput(0, graph.size() - 1);

    prompt << "Enter end vertex: ";
    unsigned end = getIntInput(0, graph.size() - 1);

    prompt << "Enter K: ";
    unsigned k = getIntInput(1);

    unsigned maxThreads;
    read32(s, &maxThreads);

    prompt << "Enter thread count (1-" << maxThreads << "): ";
    unsigned threads = getIntInput(1, maxThreads);

    sendGraph(s, graph);
    sendUint(s, start);
    sendUint(s, end);
    sendUint(s, k);
    sendUint(s, threads);

    std::vector<std::vector<unsigned>> paths;

    if (!readPaths(s, paths)) {
        close(s);
        return -1;
    }

    float time = -1;
    if (!read32<float>(s, &time)) {
        std::cout << "Could not read time from the server!\n";
    }

    close(s);

    if (paths.size() == 0) {
        std::cout << "No path found!\n";
        return 0;
    }

    if (paths.size() < k) {
        std::cout << "Only " << paths.size() << " path/s found.\n";
    }

    std::cout << "Top " << paths.size() << " shortest paths:\n";

    for (size_t i = 0; i < paths.size(); ++i) {
        unsigned cost = 0;

        for (size_t j = 0; j + 1 < paths[i].size(); ++j) {
            unsigned u = paths[i][j];
            unsigned v = paths[i][j + 1];
            
            for (auto& e : graph[u]) {
                if (e.first == v) { 
                    cost += e.second; 
                    break; 
                }
            }
        }

        std::cout << "Path " << i + 1 << ": ";

        for (unsigned node : paths[i]) {
            std::cout << node << " ";
        }
        
        std::cout << "(cost = " << cost << ")\n";
    }

    if (time >= 0) {
        std::cout << "The algorithm took " << time << "ms.\n";
    }
    
    return 0;
}