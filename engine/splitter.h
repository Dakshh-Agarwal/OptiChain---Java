#pragma once
#include "graph.h"
#include "fenwick.h"
#include <vector>
#include <string>

// ──────────────────────────────────────────────
// Split Plan — one warehouse's contribution
// ──────────────────────────────────────────────
struct SplitEntry {
    int warehouseId;
    std::string warehouseName;
    std::string city;
    int units;
    double costPerUnit;
    double totalCost;
    double deliveryTime;
    std::vector<int> routePath;
};

// ──────────────────────────────────────────────
// Full Split Result
// ──────────────────────────────────────────────
struct SplitResult {
    bool feasible;
    double totalCost;
    double maxDeliveryTime;
    std::vector<SplitEntry> plan;
};

// ──────────────────────────────────────────────
// Order Splitting Engine — Partition DP
//
//   When no single warehouse can fulfill the full order,
//   we split across multiple warehouses to minimize total cost.
//
//   dp[i] = minimum cost to fulfill exactly i units
//   using any combination of warehouses
//
//   This is a variant of the unbounded knapsack / coin change DP.
//   Complexity: O(totalUnits × numWarehouses)
// ──────────────────────────────────────────────
class OrderSplitter {
public:
    OrderSplitter(const Graph& graph, const InventoryManager& inv);

    SplitResult splitOrder(
        const std::string& productId,
        int totalUnits,
        int destinationId
    ) const;

private:
    const Graph& graph_;
    const InventoryManager& inv_;
};
