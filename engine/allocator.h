#pragma once
#include "graph.h"
#include "fenwick.h"
#include <vector>
#include <string>

// ──────────────────────────────────────────────
// Allocation Result
// ──────────────────────────────────────────────
struct AllocationResult {
    int warehouseId;
    std::string warehouseName;
    std::string city;
    double deliveryCost;
    double deliveryTime;
    int availableStock;
    std::vector<int> routePath;
};

// ──────────────────────────────────────────────
// Order Allocator
//   Uses Min-Heap to rank warehouses by cost
//   Combines Fenwick Tree stock query + Dijkstra routing
// ──────────────────────────────────────────────
class OrderAllocator {
public:
    OrderAllocator(const Graph& graph, const InventoryManager& inv);

    // Find best warehouse for an order — uses min-heap over Dijkstra costs
    // Returns top K candidates sorted by delivery cost
    std::vector<AllocationResult> findBestWarehouses(
        const std::string& productId,
        int quantity,
        int destinationId,
        int topK = 5
    ) const;

private:
    const Graph& graph_;
    const InventoryManager& inv_;
};
