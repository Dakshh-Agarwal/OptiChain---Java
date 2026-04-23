package com.optiflow.engine;

import java.util.*;

/**
 * ──────────────────────────────────────────────
 * Order Allocator — Min-Heap Priority Ranking
 *
 * Algorithm:
 *   1. Query Fenwick Tree for all warehouses with sufficient stock
 *   2. Run Dijkstra from each candidate → destination
 *   3. Push results into Min-Heap (priority queue)
 *   4. Pop top K cheapest options
 *
 * Complexity: O(W * (V+E) log V) where W = candidate warehouses
 * ──────────────────────────────────────────────
 */
public class OrderAllocator {

    public static class AllocationResult {
        public int warehouseId;
        public String warehouseName;
        public String city;
        public double deliveryCost;
        public double deliveryTime;
        public int availableStock;
        public List<Integer> routePath;

        public AllocationResult() {
            routePath = new ArrayList<>();
        }
    }

    private final Graph graph;
    private final InventoryManager inv;

    public OrderAllocator(Graph graph, InventoryManager inv) {
        this.graph = graph;
        this.inv = inv;
    }

    /**
     * Find best warehouse for an order — uses min-heap over Dijkstra costs.
     * Returns top K candidates sorted by delivery cost.
     */
    public List<AllocationResult> findBestWarehouses(String productId, int quantity, int destinationId, int topK) {
        // Step 1: Get warehouses with enough stock using Fenwick Tree
        List<int[]> candidates = inv.getWarehousesWithStock(productId, 1); // at least 1 unit

        // Min-heap: (cost, index into results)
        PriorityQueue<double[]> minHeap = new PriorityQueue<>(Comparator.comparingDouble(a -> a[0]));

        List<AllocationResult> allResults = new ArrayList<>();

        // Step 2: Run Dijkstra for each candidate
        for (int[] pair : candidates) {
            int whId = pair[0];
            int stock = pair[1];
            Graph.PathResult path = graph.dijkstra(whId, destinationId);
            if (!path.reachable) continue;

            AllocationResult ar = new AllocationResult();
            ar.warehouseId = whId;
            ar.deliveryCost = path.totalCost;
            ar.deliveryTime = path.totalTime;
            ar.availableStock = stock;
            ar.routePath = path.path;

            // Lookup warehouse info
            Graph.Warehouse wh = graph.getWarehouses().get(whId);
            if (wh != null) {
                ar.warehouseName = wh.name;
                ar.city = wh.city;
            }

            int idx = allResults.size();
            allResults.add(ar);

            // Step 3: Push into min-heap
            minHeap.offer(new double[]{ar.deliveryCost, idx});
        }

        // Step 4: Extract top K
        List<AllocationResult> result = new ArrayList<>();
        while (!minHeap.isEmpty() && result.size() < topK) {
            double[] top = minHeap.poll();
            int idx = (int) top[1];
            result.add(allResults.get(idx));
        }

        return result;
    }
}
