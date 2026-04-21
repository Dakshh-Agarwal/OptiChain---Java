#include "demand.h"
#include <numeric>

// ──────────────────────────────────────────────
// Demand Tracker Implementation
//
// Weighted Moving Average formula:
//   weight[i] = (i+1) / sum(1..N)
//   trend = Σ(demand[i] × weight[i])
//
// Recent entries have higher weights, making the
// tracker responsive to demand spikes.
//
// The deque automatically evicts old entries when
// it exceeds the window size — O(1) amortized.
// ──────────────────────────────────────────────

DemandTracker::DemandTracker(int windowSize, double restockThreshold)
    : windowSize_(windowSize), restockThreshold_(restockThreshold) {}

void DemandTracker::recordDemand(const std::string& productId, int quantity) {
    auto& dq = demandHistory_[productId];
    dq.push_back(quantity);

    // Sliding window: evict oldest if over window size
    while ((int)dq.size() > windowSize_) {
        dq.pop_front();
    }
}

double DemandTracker::computeWeightedAverage(const std::deque<int>& history) const {
    if (history.empty()) return 0.0;

    int n = (int)history.size();
    // Weight sum = 1 + 2 + ... + n = n*(n+1)/2
    double weightSum = n * (n + 1.0) / 2.0;

    double result = 0.0;
    for (int i = 0; i < n; i++) {
        double weight = (i + 1.0) / weightSum;
        result += history[i] * weight;
    }
    return result;
}

DemandTrend DemandTracker::getTrend(const std::string& productId) const {
    DemandTrend trend;
    trend.productId = productId;
    trend.weightedAverage = 0;
    trend.totalDemand = 0;
    trend.windowSize = windowSize_;
    trend.needsRestock = false;

    auto it = demandHistory_.find(productId);
    if (it == demandHistory_.end()) return trend;

    const auto& dq = it->second;
    trend.weightedAverage = computeWeightedAverage(dq);
    trend.totalDemand = std::accumulate(dq.begin(), dq.end(), 0.0);
    trend.needsRestock = (trend.weightedAverage > restockThreshold_);

    for (int v : dq) trend.recentDemand.push_back(v);

    return trend;
}

std::vector<DemandTrend> DemandTracker::getRestockAlerts() const {
    std::vector<DemandTrend> alerts;
    for (auto& pair : demandHistory_) {
        const std::string& pid = pair.first;
        DemandTrend t = getTrend(pid);
        if (t.needsRestock) {
            alerts.push_back(t);
        }
    }
    return alerts;
}

std::vector<std::string> DemandTracker::getTrackedProducts() const {
    std::vector<std::string> products;
    for (auto& pair : demandHistory_) {
        const std::string& pid = pair.first;
        products.push_back(pid);
    }
    return products;
}
