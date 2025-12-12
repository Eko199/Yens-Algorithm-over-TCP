#include <climits>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <vector>

template <typename T>
bool read32(int fd, T* value) {
    static_assert(sizeof(T) == 4, "Value must be 32-bit.");
    uint32_t networkValue;
    char* buf = reinterpret_cast<char*>(&networkValue);
    int count = 0;
    size_t total = 0;

    while (total < sizeof(T) && (count = read(fd, buf + total, sizeof(T) - total))) {
        total += count;
    }

    if (count < 0) {
        perror("read");
    }

    *value = static_cast<T>(ntohl(networkValue));
    return total == sizeof(T);
}

bool readPath(const int fd, std::vector<unsigned>& p) {
    uint32_t n;
    if (!read32<uint32_t>(fd, &n)) {
        std::cout << "There was an error when reading the result.\n";
        return false;
    }

    p = std::vector<unsigned>(n);

    for (size_t i = 0; i < n; ++i) {
        if (!read32<uint32_t>(fd, &p[i])) {
            std::cout << "There was an error when reading the result.\n";
            return false;
        }
    }

    return true;
}

bool readPaths(const int fd, std::vector<std::vector<unsigned>>& paths) {
    int32_t n;
    if (!read32<int32_t>(fd, &n)) {
        std::cout << "There was an error when reading the result.\n";
        return false;
    }

    if (n < 0) {
        std::cout << "The server returned with an error:\n";
        char buffer[1024];
        ssize_t count;

        while ((count = read(fd, buffer, sizeof(buffer)))) {
            if (write(1, buffer, count) < 0) {
                perror("write");
                return false;
            }
        }

        if (count < 0) {
            perror("read");
        }

        return false;
    }

    paths = std::vector<std::vector<unsigned>>(n);

    for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
        if (!readPath(fd, paths[i])) {
            return false;
        }
    }
    return true;
}

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
    std::cout << "How many vertices does the graph have? ";
    unsigned n = static_cast<unsigned>(getIntInput(1));

    std::vector<std::vector<std::pair<unsigned, unsigned>>> graph(n);

    for (size_t i = 0; i < n; ++i) {
        std::cout << "Enter number of edges from vertex " << i << ": ";
        unsigned deg = getIntInput(0);

        graph[i] = std::vector<std::pair<unsigned, unsigned>>(deg);

        for (size_t j = 0; j < deg; ++j) {
            std::cout << j + 1 << ") vertex: ";
            unsigned u = getIntInput(0, n - 1);

            std::cout << j + 1 << ") weight: ";
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
    int s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr = { AF_INET, htons(4095), 0 };

    if (connect(s, (const struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(s);
        return -1;
    }

    std::vector<std::vector<std::pair<unsigned, unsigned>>> graph = getGraphInput();

    std::cout << "Enter start vertex: ";
    unsigned start = getIntInput(0, graph.size() - 1);

    std::cout << "Enter end vertex: ";
    unsigned end = getIntInput(0, graph.size() - 1);

    std::cout << "Enter K: ";
    unsigned k = getIntInput(1);

    sendGraph(s, graph);
    sendUint(s, start);
    sendUint(s, end);
    sendUint(s, k);

    std::vector<std::vector<unsigned>> paths;

    if (!readPaths(s, paths)) {
        close(s);
        return -1;
    }

    close(s);

    std::cout << "Top " << k << " shortest paths:\n";
    for (size_t i = 0; i < paths.size(); ++i) {
        int cost = 0;

        for (size_t j = 0; j + 1 < paths[i].size(); ++j) {
            unsigned u = paths[i][j];
            unsigned v = paths[i][j+1];
            
            for (auto& e : graph[u]) {
                if (e.first == v) { 
                    cost += e.second; break; 
                }
            }
        }

        std::cout << "Path " << i+1 << ": ";

        for (int node : paths[i]) {
            std::cout << node << " ";
        }
        
        std::cout << "(cost = " << cost << ")\n";
    }

    return 0;
}