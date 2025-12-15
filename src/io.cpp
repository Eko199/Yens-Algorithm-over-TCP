#include <iostream>
#include <unistd.h>
#include "io.hpp"

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

bool sendPath(const int fd, const std::vector<unsigned>& p) {
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

bool sendPaths(const int fd, const std::vector<std::vector<unsigned>>& paths) {
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

bool readGraph(const int fd, std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& graph) {
    uint32_t n;

    if (!read32<uint32_t>(fd, &n)) {
        return false;
    }

    graph = std::vector<std::vector<std::pair<uint32_t, uint32_t>>>(n);

    for (size_t i = 0; i < n; ++i) {
        uint32_t deg;

        if (!read32<uint32_t>(fd, &deg)) {
            return false;
        }

        graph[i] = std::vector<std::pair<uint32_t, uint32_t>>(deg);

        for (size_t j = 0; j < deg; ++j) {
            uint32_t u, w;

            if (!read32<uint32_t>(fd, &u) || !read32<uint32_t>(fd, &w)) {
                return false;
            }

            graph[i][j] = { u, w };
        }
    }

    return true;
}