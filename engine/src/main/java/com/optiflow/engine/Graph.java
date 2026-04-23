package com.optiflow.engine;

import java.io.*;
import java.util.*;

/**
 * ──────────────────────────────────────────────
 * Logistics Graph Engine
 *   - Adjacency List representation
 *   - Dijkstra's shortest path: O((V+E) log V)
 *   - BFS shortest hop path:    O(V+E)
 * ──────────────────────────────────────────────
 */
public class Graph {

    // ── Data Classes ──

    public static class Edge {
        public int to;
        public double cost;       // delivery cost in ₹
        public double timeHours;  // transit time

        public Edge(int to, double cost, double timeHours) {
            this.to = to;
            this.cost = cost;
            this.timeHours = timeHours;
        }
    }

    public static class PathResult {
        public double totalCost;
        public double totalTime;
        public List<Integer> path;  // sequence of warehouse IDs
        public boolean reachable;

        public PathResult() {
            this.totalCost = 0;
            this.totalTime = 0;
            this.path = new ArrayList<>();
            this.reachable = false;
        }
    }

    public static class Warehouse {
        public int id;
        public String name;
        public String city;
        public double lat, lng;
        public int capacity;
    }

    // ── Fields ──

    private final Map<Integer, Warehouse> warehouses = new LinkedHashMap<>();
    private final List<List<Edge>> adj = new ArrayList<>();
    private int maxId = 0;

    // ── Graph Construction ──

    private void ensureSize(int id) {
        if (id > maxId) {
            maxId = id;
            while (adj.size() <= maxId) {
                adj.add(new ArrayList<>());
            }
        }
    }

    public void addWarehouse(Warehouse w) {
        ensureSize(w.id);
        warehouses.put(w.id, w);
    }

    public void addEdge(int from, int to, double cost, double timeHours) {
        ensureSize(from);
        ensureSize(to);
        // Bidirectional edges
        adj.get(from).add(new Edge(to, cost, timeHours));
        adj.get(to).add(new Edge(from, cost, timeHours));
    }

    // ── Dijkstra's Algorithm — O((V+E) log V) ──
    //   Uses min-heap (priority queue) to greedily
    //   expand the cheapest frontier node.

    public PathResult dijkstra(int src, int dst) {
        PathResult result = new PathResult();

        int n = adj.size();
        if (src < 0 || src >= n || dst < 0 || dst >= n) return result;

        double[] dist = new double[n];
        double[] timeArr = new double[n];
        int[] prev = new int[n];
        Arrays.fill(dist, Double.MAX_VALUE);
        Arrays.fill(timeArr, Double.MAX_VALUE);
        Arrays.fill(prev, -1);

        // Min-heap: (cost, node)
        PriorityQueue<double[]> pq = new PriorityQueue<>(Comparator.comparingDouble(a -> a[0]));

        dist[src] = 0;
        timeArr[src] = 0;
        pq.offer(new double[]{0, src});

        while (!pq.isEmpty()) {
            double[] top = pq.poll();
            double d = top[0];
            int u = (int) top[1];

            if (d > dist[u]) continue;  // stale entry
            if (u == dst) break;        // found shortest

            for (Edge edge : adj.get(u)) {
                double newDist = dist[u] + edge.cost;
                if (newDist < dist[edge.to]) {
                    dist[edge.to] = newDist;
                    timeArr[edge.to] = timeArr[u] + edge.timeHours;
                    prev[edge.to] = u;
                    pq.offer(new double[]{newDist, edge.to});
                }
            }
        }

        if (dist[dst] == Double.MAX_VALUE) return result;

        // Reconstruct path by backtracking through prev[]
        result.reachable = true;
        result.totalCost = dist[dst];
        result.totalTime = timeArr[dst];

        int cur = dst;
        while (cur != -1) {
            result.path.add(cur);
            cur = prev[cur];
        }
        Collections.reverse(result.path);

        return result;
    }

    // ── BFS — O(V+E) shortest path by hop count ──
    //   Treats all edges as weight=1

    public PathResult bfs(int src, int dst) {
        PathResult result = new PathResult();

        int n = adj.size();
        if (src < 0 || src >= n || dst < 0 || dst >= n) return result;

        int[] prev = new int[n];
        boolean[] visited = new boolean[n];
        Arrays.fill(prev, -1);

        visited[src] = true;
        Queue<Integer> q = new LinkedList<>();
        q.add(src);

        while (!q.isEmpty()) {
            int u = q.poll();
            if (u == dst) break;

            for (Edge edge : adj.get(u)) {
                if (!visited[edge.to]) {
                    visited[edge.to] = true;
                    prev[edge.to] = u;
                    q.add(edge.to);
                }
            }
        }

        if (!visited[dst]) return result;

        result.reachable = true;
        int cur = dst;
        while (cur != -1) {
            result.path.add(cur);
            cur = prev[cur];
        }
        Collections.reverse(result.path);

        // Sum actual costs along the BFS path
        for (int i = 0; i + 1 < result.path.size(); i++) {
            int from = result.path.get(i);
            int to = result.path.get(i + 1);
            for (Edge e : adj.get(from)) {
                if (e.to == to) {
                    result.totalCost += e.cost;
                    result.totalTime += e.timeHours;
                    break;
                }
            }
        }

        return result;
    }

    // ── All nodes within cost threshold using modified Dijkstra ──

    public List<int[]> withinCostThreshold(int src, double maxCost) {
        List<int[]> result = new ArrayList<>();
        int n = adj.size();
        if (src < 0 || src >= n) return result;

        double[] dist = new double[n];
        Arrays.fill(dist, Double.MAX_VALUE);

        PriorityQueue<double[]> pq = new PriorityQueue<>(Comparator.comparingDouble(a -> a[0]));
        dist[src] = 0;
        pq.offer(new double[]{0, src});

        while (!pq.isEmpty()) {
            double[] top = pq.poll();
            double d = top[0];
            int u = (int) top[1];
            if (d > dist[u]) continue;
            if (d > maxCost) break;

            if (u != src && warehouses.containsKey(u)) {
                result.add(new int[]{u, (int) d});
            }

            for (Edge edge : adj.get(u)) {
                double nd = dist[u] + edge.cost;
                if (nd < dist[edge.to] && nd <= maxCost) {
                    dist[edge.to] = nd;
                    pq.offer(new double[]{nd, edge.to});
                }
            }
        }

        return result;
    }

    // ── CSV Loaders ──

    private static String trim(String s) {
        return s == null ? "" : s.trim();
    }

    public void loadWarehouses(String path) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            br.readLine(); // skip header
            String line;
            while ((line = br.readLine()) != null) {
                String[] tokens = line.split(",");
                Warehouse w = new Warehouse();
                w.id = Integer.parseInt(trim(tokens[0]));
                w.name = trim(tokens[1]);
                w.city = trim(tokens[2]);
                w.lat = Double.parseDouble(trim(tokens[3]));
                w.lng = Double.parseDouble(trim(tokens[4]));
                w.capacity = Integer.parseInt(trim(tokens[5]));
                addWarehouse(w);
            }
        }
    }

    public void loadRoutes(String path) throws IOException {
        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            br.readLine(); // skip header
            String line;
            while ((line = br.readLine()) != null) {
                String[] tokens = line.split(",");
                int from = Integer.parseInt(trim(tokens[0]));
                int to = Integer.parseInt(trim(tokens[1]));
                double cost = Double.parseDouble(trim(tokens[2]));
                double timeHours = Double.parseDouble(trim(tokens[3]));
                addEdge(from, to, cost, timeHours);
            }
        }
    }

    // ── Accessors ──

    public Map<Integer, Warehouse> getWarehouses() {
        return warehouses;
    }

    public List<List<Edge>> getAdj() {
        return adj;
    }

    public int nodeCount() {
        return maxId + 1;
    }
}
