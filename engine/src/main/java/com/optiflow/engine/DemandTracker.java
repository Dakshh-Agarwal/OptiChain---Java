package com.optiflow.engine;

import java.util.*;

/**
 * ──────────────────────────────────────────────
 * Demand Trend Tracker
 *   Uses a Sliding Window Deque per product to track
 *   recent order quantities and compute weighted trends.
 *
 *   - Deque stores last N days of demand
 *   - Weighted Moving Average: recent days have higher weight
 *   - Flags products exceeding restock threshold
 *
 *   Operations:
 *     recordDemand(productId, qty)  → O(1) amortized
 *     getTrend(productId)           → O(window_size)
 * ──────────────────────────────────────────────
 */
public class DemandTracker {

    public static class DemandTrend {
        public String productId;
        public double weightedAverage;
        public double totalDemand;
        public int windowSize;
        public boolean needsRestock;
        public List<Integer> recentDemand = new ArrayList<>();
    }

    private final int windowSize;
    private final double restockThreshold;
    private final Map<String, ArrayDeque<Integer>> demandHistory = new LinkedHashMap<>();

    public DemandTracker(int windowSize, double restockThreshold) {
        this.windowSize = windowSize;
        this.restockThreshold = restockThreshold;
    }

    /** Record a new day's demand for a product */
    public void recordDemand(String productId, int quantity) {
        ArrayDeque<Integer> dq = demandHistory.computeIfAbsent(productId, k -> new ArrayDeque<>());
        dq.addLast(quantity);
        // Sliding window: evict oldest if over window size
        while (dq.size() > windowSize) {
            dq.pollFirst();
        }
    }

    /**
     * Weighted Moving Average formula:
     *   weight[i] = (i+1) / sum(1..N)
     *   trend = Σ(demand[i] × weight[i])
     * Recent entries have higher weights.
     */
    private double computeWeightedAverage(ArrayDeque<Integer> history) {
        if (history.isEmpty()) return 0.0;
        int n = history.size();
        double weightSum = n * (n + 1.0) / 2.0;
        double result = 0.0;
        int i = 0;
        for (int val : history) {
            double weight = (i + 1.0) / weightSum;
            result += val * weight;
            i++;
        }
        return result;
    }

    /** Get trend for a specific product */
    public DemandTrend getTrend(String productId) {
        DemandTrend trend = new DemandTrend();
        trend.productId = productId;
        trend.windowSize = windowSize;

        ArrayDeque<Integer> dq = demandHistory.get(productId);
        if (dq == null || dq.isEmpty()) return trend;

        trend.weightedAverage = computeWeightedAverage(dq);
        double total = 0;
        for (int v : dq) { total += v; trend.recentDemand.add(v); }
        trend.totalDemand = total;
        trend.needsRestock = (trend.weightedAverage > restockThreshold);
        return trend;
    }

    /** Get all products that need restocking */
    public List<DemandTrend> getRestockAlerts() {
        List<DemandTrend> alerts = new ArrayList<>();
        for (String pid : demandHistory.keySet()) {
            DemandTrend t = getTrend(pid);
            if (t.needsRestock) alerts.add(t);
        }
        return alerts;
    }
}
