#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <limits>

// ──────────────────────────────────────────────
// Graph Edge — represents a route between warehouses
// ──────────────────────────────────────────────
struct Edge {
    int to;
    double cost;        // delivery cost in ₹
    double time_hours;  // transit time
};

// ──────────────────────────────────────────────
// Dijkstra path result
// ──────────────────────────────────────────────
struct PathResult {
    double total_cost;
    double total_time;
    std::vector<int> path;  // sequence of warehouse IDs
    bool reachable;
};

// ──────────────────────────────────────────────
// Warehouse node
// ──────────────────────────────────────────────
struct Warehouse {
    int id;
    std::string name;
    std::string city;
    double lat, lng;
    int capacity;
};

// ──────────────────────────────────────────────
// Logistics Graph Engine
//   - Adjacency List representation
//   - Dijkstra's shortest path: O((V+E) log V)
//   - BFS shortest hop path:    O(V+E)
// ──────────────────────────────────────────────
class Graph {
public:
    Graph() = default;

    void addWarehouse(const Warehouse& w);
    void addEdge(int from, int to, double cost, double time_hours);

    // Dijkstra — minimum cost path between two warehouses
    PathResult dijkstra(int src, int dst) const;

    // BFS — minimum hop path (unweighted)
    PathResult bfs(int src, int dst) const;

    // All warehouses reachable within a cost threshold from src
    std::vector<std::pair<int, double>> withinCostThreshold(int src, double maxCost) const;

    // CSV loaders
    void loadWarehouses(const std::string& path);
    void loadRoutes(const std::string& path);

    const std::unordered_map<int, Warehouse>& getWarehouses() const { return warehouses_; }
    const std::vector<std::vector<Edge>>& getAdj() const { return adj_; }
    int nodeCount() const { return maxId_ + 1; }

private:
    std::unordered_map<int, Warehouse> warehouses_;
    std::vector<std::vector<Edge>> adj_;
    int maxId_ = 0;

    void ensureSize(int id);
};
