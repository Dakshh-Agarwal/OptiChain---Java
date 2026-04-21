#include "graph.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>

// ──────────────────────────────────────────────
// Internal helpers
// ──────────────────────────────────────────────
void Graph::ensureSize(int id) {
    if (id > maxId_) {
        maxId_ = id;
        adj_.resize(maxId_ + 1);
    }
}

// Trim whitespace
static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ──────────────────────────────────────────────
// Graph construction
// ──────────────────────────────────────────────
void Graph::addWarehouse(const Warehouse& w) {
    ensureSize(w.id);
    warehouses_[w.id] = w;
}

void Graph::addEdge(int from, int to, double cost, double time_hours) {
    ensureSize(from);
    ensureSize(to);
    // Bidirectional edges
    adj_[from].push_back({to, cost, time_hours});
    adj_[to].push_back({from, cost, time_hours});
}

// ──────────────────────────────────────────────
// Dijkstra's Algorithm — O((V+E) log V)
//   Uses min-heap (priority queue) to greedily
//   expand the cheapest frontier node.
// ──────────────────────────────────────────────
PathResult Graph::dijkstra(int src, int dst) const {
    PathResult result;
    result.reachable = false;
    result.total_cost = 0;
    result.total_time = 0;

    int n = (int)adj_.size();
    if (src < 0 || src >= n || dst < 0 || dst >= n) return result;

    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(n, INF);
    std::vector<double> timeArr(n, INF);
    std::vector<int> prev(n, -1);

    // Min-heap: (cost, node)
    using PII = std::pair<double, int>;
    std::priority_queue<PII, std::vector<PII>, std::greater<PII>> pq;

    dist[src] = 0;
    timeArr[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto topPair = pq.top();
        double d = topPair.first;
        int u = topPair.second;
        pq.pop();

        if (d > dist[u]) continue;  // stale entry
        if (u == dst) break;        // found shortest

        for (const auto& edge : adj_[u]) {
            double newDist = dist[u] + edge.cost;
            if (newDist < dist[edge.to]) {
                dist[edge.to] = newDist;
                timeArr[edge.to] = timeArr[u] + edge.time_hours;
                prev[edge.to] = u;
                pq.push({newDist, edge.to});
            }
        }
    }

    if (dist[dst] == INF) return result;

    // Reconstruct path by backtracking through prev[]
    result.reachable = true;
    result.total_cost = dist[dst];
    result.total_time = timeArr[dst];

    int cur = dst;
    while (cur != -1) {
        result.path.push_back(cur);
        cur = prev[cur];
    }
    std::reverse(result.path.begin(), result.path.end());

    return result;
}

// ──────────────────────────────────────────────
// BFS — O(V+E) shortest path by hop count
//   Treats all edges as weight=1
// ──────────────────────────────────────────────
PathResult Graph::bfs(int src, int dst) const {
    PathResult result;
    result.reachable = false;
    result.total_cost = 0;
    result.total_time = 0;

    int n = (int)adj_.size();
    if (src < 0 || src >= n || dst < 0 || dst >= n) return result;

    std::vector<int> prev(n, -1);
    std::vector<bool> visited(n, false);
    std::queue<int> q;

    visited[src] = true;
    q.push(src);

    while (!q.empty()) {
        int u = q.front();
        q.pop();
        if (u == dst) break;

        for (const auto& edge : adj_[u]) {
            if (!visited[edge.to]) {
                visited[edge.to] = true;
                prev[edge.to] = u;
                q.push(edge.to);
            }
        }
    }

    if (!visited[dst]) return result;

    result.reachable = true;
    int cur = dst;
    while (cur != -1) {
        result.path.push_back(cur);
        cur = prev[cur];
    }
    std::reverse(result.path.begin(), result.path.end());

    // Sum actual costs along the BFS path
    for (size_t i = 0; i + 1 < result.path.size(); i++) {
        int from = result.path[i];
        int to = result.path[i + 1];
        for (const auto& e : adj_[from]) {
            if (e.to == to) {
                result.total_cost += e.cost;
                result.total_time += e.time_hours;
                break;
            }
        }
    }

    return result;
}

// ──────────────────────────────────────────────
// All nodes within cost threshold using modified Dijkstra
// ──────────────────────────────────────────────
std::vector<std::pair<int, double>> Graph::withinCostThreshold(int src, double maxCost) const {
    std::vector<std::pair<int, double>> result;
    int n = (int)adj_.size();
    if (src < 0 || src >= n) return result;

    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dist(n, INF);

    using PII = std::pair<double, int>;
    std::priority_queue<PII, std::vector<PII>, std::greater<PII>> pq;

    dist[src] = 0;
    pq.push({0, src});

    while (!pq.empty()) {
        auto topPair = pq.top();
        double d = topPair.first;
        int u = topPair.second;
        pq.pop();
        if (d > dist[u]) continue;
        if (d > maxCost) break;

        if (u != src && warehouses_.count(u)) {
            result.push_back({u, d});
        }

        for (const auto& edge : adj_[u]) {
            double nd = dist[u] + edge.cost;
            if (nd < dist[edge.to] && nd <= maxCost) {
                dist[edge.to] = nd;
                pq.push({nd, edge.to});
            }
        }
    }

    return result;
}

// ──────────────────────────────────────────────
// CSV Loaders
// ──────────────────────────────────────────────
void Graph::loadWarehouses(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        Warehouse w;

        std::getline(ss, token, ','); w.id = std::stoi(trim(token));
        std::getline(ss, token, ','); w.name = trim(token);
        std::getline(ss, token, ','); w.city = trim(token);
        std::getline(ss, token, ','); w.lat = std::stod(trim(token));
        std::getline(ss, token, ','); w.lng = std::stod(trim(token));
        std::getline(ss, token, ','); w.capacity = std::stoi(trim(token));

        addWarehouse(w);
    }
}

void Graph::loadRoutes(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;

        int from, to;
        double cost, time_hours;

        std::getline(ss, token, ','); from = std::stoi(trim(token));
        std::getline(ss, token, ','); to = std::stoi(trim(token));
        std::getline(ss, token, ','); cost = std::stod(trim(token));
        std::getline(ss, token, ','); time_hours = std::stod(trim(token));
        // skip distance_km col

        addEdge(from, to, cost, time_hours);
    }
}
