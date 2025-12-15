#include <cstdint>
#include <cstdio>
#include <netinet/in.h>
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

    *value = std::bit_cast<T>(ntohl(networkValue));
    return total == sizeof(T);
}

template <typename T>
ssize_t send32(const int fd, const T value) {
    static_assert(sizeof(T) == 4, "Value must be 32-bit.");
    uint32_t nToSend = htonl(std::bit_cast<uint32_t>(value));
    return write(fd, &nToSend, sizeof(uint32_t));
}

bool readPath(const int fd, std::vector<unsigned>& p);
bool sendPath(const int fd, const std::vector<unsigned>& p);

bool readPaths(const int fd, std::vector<std::vector<unsigned>>& paths);
bool sendPaths(const int fd, const std::vector<std::vector<unsigned>>& paths);

bool readGraph(const int fd, std::vector<std::vector<std::pair<uint32_t, uint32_t>>>& graph);