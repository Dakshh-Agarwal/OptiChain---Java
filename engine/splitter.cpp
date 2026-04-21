#include "splitter.h"
#include <algorithm>
#include <limits>
#include <cmath>

// ──────────────────────────────────────────────
// Order Splitting — Partition DP
//
//   We model this as a variant of the bounded knapsack problem:
//
//   For each warehouse j with stock s_j and cost-per-unit c_j:
//     For i from totalUnits down to 1:
//       For k from 1 to min(i, s_j):
//         dp[i] = min(dp[i], dp[i-k] + k * c_j)
//
//   c_j = deliveryCost(warehouse_j → destination) / stock_j
//   (normalized cost per unit from that warehouse)
//
//   We then backtrack to find which warehouses contribute how many units.
//
//   Complexity: O(U × W × min(U, maxStock))
//   where U = total units, W = warehouse count
// ──────────────────────────────────────────────

OrderSplitter::OrderSplitter(const Graph& graph, const InventoryManager& inv)
    : graph_(graph), inv_(inv) {}

SplitResult OrderSplitter::splitOrder(
    const std::string& productId,
    int totalUnits,
    int destinationId) const
{
    SplitResult result;
    result.feasible = false;
    result.totalCost = 0;
    result.maxDeliveryTime = 0;

    // Get all warehouses with any stock of this product
    auto candidates = inv_.getWarehousesWithStock(productId, 1);
    if (candidates.empty()) return result;

    // Pre-compute routing cost for each candidate
    struct CandidateInfo {
        int whId;
        int stock;
        double routeCost;     // total route cost
        double routeTime;
        double costPerUnit;   // routeCost spread across available units
        std::vector<int> path;
        std::string name;
        std::string city;
    };

    std::vector<CandidateInfo> infos;
    for (auto& pair : candidates) {
        int whId = pair.first;
        int stock = pair.second;
        PathResult pr = graph_.dijkstra(whId, destinationId);
        if (!pr.reachable) continue;

        CandidateInfo ci;
        ci.whId = whId;
        ci.stock = stock;
        ci.routeCost = pr.total_cost;
        ci.routeTime = pr.total_time;
        ci.costPerUnit = pr.total_cost; // base cost per shipment
        ci.path = pr.path;

        auto& warehouses = graph_.getWarehouses();
        auto it = warehouses.find(whId);
        if (it != warehouses.end()) {
            ci.name = it->second.name;
            ci.city = it->second.city;
        }

        infos.push_back(ci);
    }

    if (infos.empty()) return result;

    // Check total available stock
    int totalAvailable = 0;
    for (auto& ci : infos) totalAvailable += ci.stock;
    if (totalAvailable < totalUnits) return result;

    // DP: dp[i] = minimum cost to fulfill exactly i units
    const double INF = std::numeric_limits<double>::infinity();
    std::vector<double> dp(totalUnits + 1, INF);
    // Track which warehouse contributed at each step
    // choice[i] = {warehouseIndex, unitsFromThatWarehouse}
    std::vector<std::pair<int, int>> choice(totalUnits + 1, {-1, 0});

    dp[0] = 0;

    // For each warehouse
    for (int w = 0; w < (int)infos.size(); w++) {
        auto& ci = infos[w];
        // Process in reverse to avoid using same warehouse twice
        // (bounded knapsack style)
        for (int i = totalUnits; i >= 1; i--) {
            int maxUnits = std::min(i, ci.stock);
            for (int k = 1; k <= maxUnits; k++) {
                // Cost = base route cost + k units * small per-unit handling
                double cost = ci.routeCost + k * 0.5; // ₹0.50 handling per unit
                if (dp[i - k] + cost < dp[i]) {
                    dp[i] = dp[i - k] + cost;
                    choice[i] = {w, k};
                }
            }
        }
    }

    if (dp[totalUnits] >= INF) return result;

    // Backtrack to find the split plan
    result.feasible = true;
    result.totalCost = dp[totalUnits];

    std::vector<int> unitsPerWarehouse(infos.size(), 0);
    int remaining = totalUnits;
    while (remaining > 0) {
        auto choicePair = choice[remaining];
        int wIdx = choicePair.first;
        int units = choicePair.second;
        if (wIdx == -1) { result.feasible = false; return result; }
        unitsPerWarehouse[wIdx] += units;
        remaining -= units;
    }

    // Build plan
    for (int w = 0; w < (int)infos.size(); w++) {
        if (unitsPerWarehouse[w] == 0) continue;

        SplitEntry entry;
        entry.warehouseId = infos[w].whId;
        entry.warehouseName = infos[w].name;
        entry.city = infos[w].city;
        entry.units = unitsPerWarehouse[w];
        entry.costPerUnit = infos[w].routeCost / unitsPerWarehouse[w];
        entry.totalCost = infos[w].routeCost + unitsPerWarehouse[w] * 0.5;
        entry.deliveryTime = infos[w].routeTime;
        entry.routePath = infos[w].path;

        result.maxDeliveryTime = std::max(result.maxDeliveryTime, entry.deliveryTime);
        result.plan.push_back(entry);
    }

    return result;
}
