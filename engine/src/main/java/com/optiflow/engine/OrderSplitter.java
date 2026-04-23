package com.optiflow.engine;

import java.util.*;

/**
 * ──────────────────────────────────────────────
 * Order Splitting Engine — Partition DP
 *
 *   dp[i] = minimum cost to fulfill exactly i units
 *   Bounded knapsack variant.
 *   Complexity: O(U × W × min(U, maxStock))
 * ──────────────────────────────────────────────
 */
public class OrderSplitter {

    public static class SplitEntry {
        public int warehouseId;
        public String warehouseName;
        public String city;
        public int units;
        public double costPerUnit;
        public double totalCost;
        public double deliveryTime;
        public List<Integer> routePath = new ArrayList<>();
    }

    public static class SplitResult {
        public boolean feasible = false;
        public double totalCost = 0;
        public double maxDeliveryTime = 0;
        public List<SplitEntry> plan = new ArrayList<>();
    }

    private final Graph graph;
    private final InventoryManager inv;

    public OrderSplitter(Graph graph, InventoryManager inv) {
        this.graph = graph;
        this.inv = inv;
    }

    private static class CandidateInfo {
        int whId, stock;
        double routeCost, routeTime;
        List<Integer> path;
        String name = "", city = "";
    }

    public SplitResult splitOrder(String productId, int totalUnits, int destinationId) {
        SplitResult result = new SplitResult();
        List<int[]> candidates = inv.getWarehousesWithStock(productId, 1);
        if (candidates.isEmpty()) return result;

        List<CandidateInfo> infos = new ArrayList<>();
        for (int[] pair : candidates) {
            Graph.PathResult pr = graph.dijkstra(pair[0], destinationId);
            if (!pr.reachable) continue;
            CandidateInfo ci = new CandidateInfo();
            ci.whId = pair[0]; ci.stock = pair[1];
            ci.routeCost = pr.totalCost; ci.routeTime = pr.totalTime;
            ci.path = pr.path;
            Graph.Warehouse wh = graph.getWarehouses().get(pair[0]);
            if (wh != null) { ci.name = wh.name; ci.city = wh.city; }
            infos.add(ci);
        }
        if (infos.isEmpty()) return result;

        int totalAvailable = 0;
        for (CandidateInfo ci : infos) totalAvailable += ci.stock;
        if (totalAvailable < totalUnits) return result;

        double[] dp = new double[totalUnits + 1];
        Arrays.fill(dp, Double.MAX_VALUE);
        int[][] choice = new int[totalUnits + 1][2];
        for (int[] c : choice) { c[0] = -1; c[1] = 0; }
        dp[0] = 0;

        for (int w = 0; w < infos.size(); w++) {
            CandidateInfo ci = infos.get(w);
            for (int i = totalUnits; i >= 1; i--) {
                int maxUnits = Math.min(i, ci.stock);
                for (int k = 1; k <= maxUnits; k++) {
                    double cost = ci.routeCost + k * 0.5;
                    if (dp[i - k] != Double.MAX_VALUE && dp[i - k] + cost < dp[i]) {
                        dp[i] = dp[i - k] + cost;
                        choice[i][0] = w; choice[i][1] = k;
                    }
                }
            }
        }

        if (dp[totalUnits] >= Double.MAX_VALUE) return result;

        result.feasible = true;
        result.totalCost = dp[totalUnits];
        int[] unitsPerWh = new int[infos.size()];
        int remaining = totalUnits;
        while (remaining > 0) {
            int wIdx = choice[remaining][0], units = choice[remaining][1];
            if (wIdx == -1) { result.feasible = false; return result; }
            unitsPerWh[wIdx] += units; remaining -= units;
        }

        for (int w = 0; w < infos.size(); w++) {
            if (unitsPerWh[w] == 0) continue;
            SplitEntry e = new SplitEntry();
            e.warehouseId = infos.get(w).whId;
            e.warehouseName = infos.get(w).name;
            e.city = infos.get(w).city;
            e.units = unitsPerWh[w];
            e.costPerUnit = infos.get(w).routeCost / unitsPerWh[w];
            e.totalCost = infos.get(w).routeCost + unitsPerWh[w] * 0.5;
            e.deliveryTime = infos.get(w).routeTime;
            e.routePath = infos.get(w).path;
            result.maxDeliveryTime = Math.max(result.maxDeliveryTime, e.deliveryTime);
            result.plan.add(e);
        }
        return result;
    }
}
