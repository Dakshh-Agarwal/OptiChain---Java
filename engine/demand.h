#pragma once
#include <deque>
#include <unordered_map>
#include <string>
#include <vector>

// ──────────────────────────────────────────────
// Demand Trend Tracker
//   Uses a Sliding Window Deque per product to track
//   recent order quantities and compute weighted trends.
//
//   - Deque stores last N days of demand
//   - Weighted Moving Average: recent days have higher weight
//   - Flags products exceeding restock threshold
//
//   Operations:
//     recordDemand(productId, qty)  → O(1) amortized
//     getTrend(productId)           → O(window_size)
// ──────────────────────────────────────────────

struct DemandTrend {
    std::string productId;
    double weightedAverage;
    double totalDemand;
    int windowSize;
    bool needsRestock;
    std::vector<int> recentDemand;
};

class DemandTracker {
public:
    explicit DemandTracker(int windowSize = 7, double restockThreshold = 50.0);

    // Record a new day's demand for a product
    void recordDemand(const std::string& productId, int quantity);

    // Get trend for a specific product
    DemandTrend getTrend(const std::string& productId) const;

    // Get all products that need restocking
    std::vector<DemandTrend> getRestockAlerts() const;

    // Get all tracked products
    std::vector<std::string> getTrackedProducts() const;

private:
    int windowSize_;
    double restockThreshold_;
    std::unordered_map<std::string, std::deque<int>> demandHistory_;

    double computeWeightedAverage(const std::deque<int>& history) const;
};
