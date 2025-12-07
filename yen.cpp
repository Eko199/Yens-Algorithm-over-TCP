#include <algorithm>
#include <climits>
#include <functional>
#include <iostream>
#include <queue>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

typedef std::pair<unsigned, unsigned> edge;
typedef std::vector<unsigned> path;

struct edgeHash {
    size_t operator()(const edge& e) const {
        size_t hash1 = std::hash<unsigned>{}(e.first);
        size_t hash2 = std::hash<unsigned>{}(e.second);

        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

struct pathHash {
    size_t operator()(const path& p) const {
        size_t h = 0;

        for (unsigned v : p) {
            h ^= std::hash<unsigned>{}(v) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        
        return h;
    }
};

struct pathWithCost {
    path pathNodes;
    std::vector<unsigned> cumulativeCost;

    unsigned getTotalCost() const {
        return cumulativeCost[cumulativeCost.size() - 1];
    }

    bool operator<(const pathWithCost& other) const {
        return getTotalCost() > other.getTotalCost();
    }
};

std::vector<unsigned> dijkstra(const std::vector<std::vector<edge>>& graph, const unsigned start, 
    std::vector<unsigned>* prev = nullptr, std::function<bool(const edge&)> filter = nullptr) {
    if (start > graph.size() - 1) {
        throw std::invalid_argument("Provided start is not a vertex in the graph.");
    }

    std::vector<unsigned> dist(graph.size(), INT_MAX);
    dist[start] = 0;

    if (prev) {
        *prev = std::vector<unsigned>(graph.size(), INT_MAX);
    }

    std::priority_queue<edge, std::vector<edge>, std::greater<edge>> pq;
    pq.push({ dist[start], start });

    while (!pq.empty()) {
        unsigned v = pq.top().second;
        unsigned d = pq.top().first;
        pq.pop();

        if (d > dist[v]) {
            continue;
        }

        for (const edge& neighbour : graph[v]) {
            if (filter && !filter({ v, neighbour.first })) {
                continue;
            }

            d = dist[v] + neighbour.second;
            
            if (d < dist[neighbour.first]) {
                dist[neighbour.first] = d;
                pq.push({ d, neighbour.first });

                if (prev) {
                    (*prev)[neighbour.first] = v;
                }
            }
        }
    }

    return dist;
}

pathWithCost dijkstra_to(const std::vector<std::vector<edge>>& graph, const unsigned start, 
    unsigned end, std::function<bool(const edge&)> filter = nullptr) {
    std::vector<unsigned> prev;
    std::vector<unsigned> dist = dijkstra(graph, start, &prev, filter);

    pathWithCost result;
    while (end != INT_MAX) {
        result.pathNodes.push_back(end);
        end = prev[end];
    }

    std::reverse(result.pathNodes.begin(), result.pathNodes.end());
    
    for (unsigned v : result.pathNodes) {
        result.cumulativeCost.push_back(dist[v]);
    }
    
    return result;
}

std::vector<path> yen(std::vector<std::vector<edge>> graph, const unsigned start, const unsigned end, const unsigned k) {
    if (start > graph.size() - 1 || end > graph.size() - 1) {
        throw std::invalid_argument("Provided start or end is not a vertex in the graph.");
    }

    std::vector<path> kth_path(k);
    std::vector<std::vector<unsigned>> kth_cost(k);

    pathWithCost path0 = dijkstra_to(graph, start, end);
    kth_path[0] = path0.pathNodes;
    kth_cost[0] = path0.cumulativeCost;

    std::priority_queue<pathWithCost> candidate_paths;
    std::unordered_set<path, pathHash> candidates_set;

    for (unsigned curr_k = 1; curr_k < k; ++curr_k) {
        path prev_path = kth_path[curr_k - 1];

        //i - deviation from k-1th shortest path
        for (unsigned i = 0; i < prev_path.size() - 1; ++i) {
            std::unordered_set<edge, edgeHash> banned_edges;

            for (const path& path : kth_path) {
                if (path.size() < i + 1 || !std::equal(prev_path.begin(), prev_path.begin() + i, path.begin())) {
                    continue;
                }

                banned_edges.insert({ path[i], path[i + 1] });
            }

            std::unordered_set<unsigned> banned_vertices;

            for (unsigned j = 0; j < i; ++j) {
                banned_vertices.insert(prev_path[j]);
            }

            std::function<bool(const edge&)> filter = [&](const edge& e) {
                return !banned_edges.count(e) && !banned_vertices.count(e.first) && !banned_vertices.count(e.second);
            };

            pathWithCost spurPath = dijkstra_to(graph, prev_path[i], end, filter);

            if (spurPath.getTotalCost() >= INT_MAX) {
                continue;
            }

            //append root to spur
            path curr_path(prev_path.begin(), prev_path.begin() + i);
            curr_path.insert(curr_path.end(), spurPath.pathNodes.begin(), spurPath.pathNodes.end());

            if (candidates_set.count(curr_path)) {
                continue;
            }

            candidates_set.insert(curr_path);
            
            std::vector<unsigned> cumulativeCost;
            for (size_t j = 0; j < curr_path.size(); ++j) {
                cumulativeCost.push_back(j <= i ? kth_cost[curr_k - 1][j] : kth_cost[curr_k - 1][i] + spurPath.cumulativeCost[j - i]);
            }
            
            candidate_paths.push({ curr_path, cumulativeCost });
        }

        if (candidates_set.empty()) {
            break;
        }

        kth_path[curr_k] = candidate_paths.top().pathNodes;
        kth_cost[curr_k] = candidate_paths.top().cumulativeCost;

        candidates_set.erase(kth_path[curr_k]);
        candidate_paths.pop();
    }

    return kth_path;
}

int main() {
    std::vector<std::vector<edge>> graph(5);

    graph[0].push_back({1, 10});
    graph[0].push_back({2, 3});

    graph[1].push_back({2, 1});
    graph[1].push_back({3, 2});

    graph[2].push_back({1, 4});
    graph[2].push_back({3, 8});
    graph[2].push_back({4, 2});

    graph[3].push_back({4, 7});

    graph[4].push_back({3, 9});

    for (unsigned i = 0; i < graph.size(); i++) {
        std::cout << "Node " << i << ": ";

        for (edge& edge : graph[i]) {
            std::cout << "(" << edge.first << ", " << edge.second << ") ";
        }
        
        std::cout << "\n";
    }

    unsigned k = 5;
    std::vector<path> paths = yen(graph, 0, 3, k);

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