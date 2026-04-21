#include "allocator.h"
#include <queue>
#include <algorithm>

// ──────────────────────────────────────────────
// Order Allocator Implementation
//
// Algorithm:
//   1. Query Fenwick Tree for all warehouses with sufficient stock
//   2. Run Dijkstra from each candidate → destination
//   3. Push results into Min-Heap (priority queue)
//   4. Pop top K cheapest options
//
// Complexity: O(W * (V+E) log V) where W = candidate warehouses
// ──────────────────────────────────────────────

OrderAllocator::OrderAllocator(const Graph& graph, const InventoryManager& inv)
    : graph_(graph), inv_(inv) {}

std::vector<AllocationResult> OrderAllocator::findBestWarehouses(
    const std::string& productId,
    int quantity,
    int destinationId,
    int topK) const
{
    // Step 1: Get warehouses with enough stock using Fenwick Tree
    auto candidates = inv_.getWarehousesWithStock(productId, 1); // at least 1 unit

    // Min-heap: (cost, index into results)
    using PII = std::pair<double, int>;
    std::priority_queue<PII, std::vector<PII>, std::greater<PII>> minHeap;

    std::vector<AllocationResult> allResults;

    // Step 2: Run Dijkstra for each candidate
    for (auto& pair : candidates) {
        int whId = pair.first;
        int stock = pair.second;
        PathResult path = graph_.dijkstra(whId, destinationId);
        if (!path.reachable) continue;

        AllocationResult ar;
        ar.warehouseId = whId;
        ar.deliveryCost = path.total_cost;
        ar.deliveryTime = path.total_time;
        ar.availableStock = stock;
        ar.routePath = path.path;

        // Lookup warehouse info
        auto& warehouses = graph_.getWarehouses();
        auto it = warehouses.find(whId);
        if (it != warehouses.end()) {
            ar.warehouseName = it->second.name;
            ar.city = it->second.city;
        }

        int idx = (int)allResults.size();
        allResults.push_back(ar);

        // Step 3: Push into min-heap
        minHeap.push({ar.deliveryCost, idx});
    }

    // Step 4: Extract top K
    std::vector<AllocationResult> result;
    while (!minHeap.empty() && (int)result.size() < topK) {
        auto topPair = minHeap.top();
        double cost = topPair.first;
        int idx = topPair.second;
        minHeap.pop();
        result.push_back(allResults[idx]);
    }

    return result;
}
