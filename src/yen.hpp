#pragma once

#include <functional>
#include <thread>
#include <utility>
#include <vector>

typedef std::pair<unsigned, unsigned> edge;
typedef std::vector<unsigned> path;

std::vector<unsigned> dijkstra(const std::vector<std::vector<edge>>& graph, const unsigned start, 
    std::vector<unsigned>* prev = nullptr, std::function<bool(const edge&)> filter = nullptr);

std::vector<path> yen(const std::vector<std::vector<edge>>& graph, const unsigned start, 
    const unsigned end, const unsigned k, const unsigned threads = std::thread::hardware_concurrency());