package com.optiflow.engine;

import com.sun.net.httpserver.*;
import java.io.*;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.stream.Collectors;

/**
 * ──────────────────────────────────────────────
 * OptiFlow HTTP Server
 *   Uses JDK built-in HttpServer (no external dependencies)
 *   Serves REST API for the React frontend
 *   All DSA computation happens in-process (no subprocess)
 * ──────────────────────────────────────────────
 */
public class OptiFlowServer {

    private static Graph graph;
    private static InventoryManager inventory;
    private static DemandTracker demandTracker;

    public static void main(String[] args) throws Exception {
        // Determine paths
        String engineDir = System.getProperty("user.dir");
        // If running from engine dir, data is ../data
        // If running from project root, data is ./data
        String dataDir;
        File dataCheck = new File(engineDir, "data");
        if (dataCheck.exists()) {
            dataDir = dataCheck.getAbsolutePath();
        } else {
            dataDir = new File(engineDir, "../data").getCanonicalPath();
        }

        System.out.println("[Engine] Data directory: " + dataDir);

        // Initialize DSA modules
        graph = new Graph();
        inventory = new InventoryManager();
        demandTracker = new DemandTracker(7, 50.0);

        graph.loadWarehouses(dataDir + File.separator + "warehouses.csv");
        graph.loadRoutes(dataDir + File.separator + "routes.csv");
        inventory.loadInventory(dataDir + File.separator + "inventory.csv", 25);

        System.out.println("[Engine] Loaded " + graph.getWarehouses().size() + " warehouses");
        System.out.println("[Engine] Loaded " + inventory.getProducts().size() + " products");

        // Get port from environment variable for deployment (Railway)
        String portEnv = System.getenv("PORT");
        int port = (portEnv != null) ? Integer.parseInt(portEnv) : 5000;

        // Start HTTP server
        HttpServer server = HttpServer.create(new InetSocketAddress(port), 0);

        server.createContext("/api/health", ex -> handle(ex, OptiFlowServer::handleHealth));
        server.createContext("/api/graph", ex -> handle(ex, OptiFlowServer::handleGraph));
        server.createContext("/api/warehouses", ex -> handle(ex, OptiFlowServer::handleWarehouses));
        server.createContext("/api/dijkstra", ex -> handle(ex, OptiFlowServer::handleDijkstra));
        server.createContext("/api/bfs", ex -> handle(ex, OptiFlowServer::handleBfs));
        server.createContext("/api/inventory/all", ex -> handle(ex, OptiFlowServer::handleAllInventory));
        server.createContext("/api/inventory", ex -> handle(ex, OptiFlowServer::handleInventory));
        server.createContext("/api/allocate", ex -> handle(ex, OptiFlowServer::handleAllocate));
        server.createContext("/api/split", ex -> handle(ex, OptiFlowServer::handleSplit));
        server.createContext("/api/demand/record", ex -> handle(ex, OptiFlowServer::handleDemandRecord));
        server.createContext("/api/demand/trend", ex -> handle(ex, OptiFlowServer::handleDemandTrend));
        server.createContext("/api/demand/alerts", ex -> handle(ex, OptiFlowServer::handleDemandAlerts));

        server.setExecutor(null);
        server.start();
        System.out.println("[Engine] OptiFlow Java server running on http://localhost:5000");
    }

    // ── Request Handler Wrapper with CORS ──

    @FunctionalInterface
    interface Handler { String handle(HttpExchange ex) throws Exception; }

    private static void handle(HttpExchange ex, Handler handler) {
        try {
            // CORS headers
            ex.getResponseHeaders().set("Access-Control-Allow-Origin", "*");
            ex.getResponseHeaders().set("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
            ex.getResponseHeaders().set("Access-Control-Allow-Headers", "Content-Type");
            ex.getResponseHeaders().set("Content-Type", "application/json");

            if ("OPTIONS".equalsIgnoreCase(ex.getRequestMethod())) {
                ex.sendResponseHeaders(204, -1);
                return;
            }

            String response = handler.handle(ex);
            byte[] bytes = response.getBytes(StandardCharsets.UTF_8);
            ex.sendResponseHeaders(200, bytes.length);
            try (OutputStream os = ex.getResponseBody()) {
                os.write(bytes);
            }
        } catch (Exception e) {
            try {
                String err = "{\"error\":\"" + escStr(e.getMessage()) + "\"}";
                byte[] bytes = err.getBytes(StandardCharsets.UTF_8);
                ex.sendResponseHeaders(500, bytes.length);
                try (OutputStream os = ex.getResponseBody()) { os.write(bytes); }
            } catch (IOException ignored) {}
        }
    }

    // ── Endpoint Handlers ──

    private static String handleHealth(HttpExchange ex) {
        return "{\"status\":\"ok\",\"engine\":true}";
    }

    private static String handleGraph(HttpExchange ex) {
        StringBuilder sb = new StringBuilder();
        sb.append("{\"nodes\":");
        appendWarehouseArray(sb);
        sb.append(",\"edges\":");
        appendEdgeArray(sb);
        sb.append("}");
        return sb.toString();
    }

    private static String handleWarehouses(HttpExchange ex) {
        StringBuilder sb = new StringBuilder();
        sb.append("{\"warehouses\":");
        appendWarehouseArray(sb);
        sb.append("}");
        return sb.toString();
    }

    private static String handleDijkstra(HttpExchange ex) throws IOException {
        Map<String, Object> body = parseJsonBody(ex);
        int from = toInt(body.get("from"));
        int to = toInt(body.get("to"));
        Graph.PathResult pr = graph.dijkstra(from, to);
        return pathResultToJson(pr);
    }

    private static String handleBfs(HttpExchange ex) throws IOException {
        Map<String, Object> body = parseJsonBody(ex);
        int from = toInt(body.get("from"));
        int to = toInt(body.get("to"));
        Graph.PathResult pr = graph.bfs(from, to);

        StringBuilder sb = new StringBuilder();
        sb.append("{\"reachable\":").append(pr.reachable);
        sb.append(",\"total_cost\":").append(pr.totalCost);
        sb.append(",\"total_time\":").append(pr.totalTime);
        sb.append(",\"hops\":").append(pr.path.size() > 0 ? pr.path.size() - 1 : 0);
        sb.append(",\"path\":").append(intListToJson(pr.path));
        sb.append("}");
        return sb.toString();
    }

    private static String handleInventory(HttpExchange ex) {
        String product = getQueryParam(ex, "product", "P001");
        StringBuilder sb = new StringBuilder();
        sb.append("{\"product\":\"").append(escStr(product)).append("\"");
        sb.append(",\"total_stock\":").append(inventory.getTotalStock(product));

        sb.append(",\"warehouses\":[");
        boolean first = true;
        for (int i = 1; i <= inventory.warehouseCount(); i++) {
            int stock = inventory.getStock(product, i);
            if (stock > 0) {
                if (!first) sb.append(",");
                first = false;
                sb.append("{\"warehouse_id\":").append(i);
                sb.append(",\"stock\":").append(stock);
                Graph.Warehouse wh = graph.getWarehouses().get(i);
                if (wh != null) {
                    sb.append(",\"city\":\"").append(escStr(wh.city)).append("\"");
                    sb.append(",\"name\":\"").append(escStr(wh.name)).append("\"");
                }
                sb.append("}");
            }
        }
        sb.append("]}");
        return sb.toString();
    }

    private static String handleAllInventory(HttpExchange ex) {
        StringBuilder sb = new StringBuilder();
        sb.append("{\"products\":[");
        boolean first = true;
        for (Map.Entry<String, InventoryManager.ProductInfo> entry : inventory.getProducts().entrySet()) {
            String pid = entry.getKey();
            InventoryManager.ProductInfo info = entry.getValue();
            if (!first) sb.append(",");
            first = false;
            sb.append("{\"id\":\"").append(escStr(info.id)).append("\"");
            sb.append(",\"name\":\"").append(escStr(info.name)).append("\"");
            sb.append(",\"unit_price\":").append(info.unitPrice);
            sb.append(",\"total_stock\":").append(inventory.getTotalStock(pid));
            sb.append(",\"warehouses\":[");
            for (int i = 1; i <= inventory.warehouseCount(); i++) {
                if (i > 1) sb.append(",");
                sb.append("{\"warehouse_id\":").append(i);
                sb.append(",\"stock\":").append(inventory.getStock(pid, i));
                sb.append("}");
            }
            sb.append("]}");
        }
        sb.append("]}");
        return sb.toString();
    }

    private static String handleAllocate(HttpExchange ex) throws IOException {
        Map<String, Object> body = parseJsonBody(ex);
        String product = (String) body.get("product");
        int qty = toInt(body.get("quantity"));
        int dest = toInt(body.get("destination"));

        OrderAllocator allocator = new OrderAllocator(graph, inventory);
        List<OrderAllocator.AllocationResult> results = allocator.findBestWarehouses(product, qty, dest, 5);

        StringBuilder sb = new StringBuilder();
        sb.append("{\"product\":\"").append(escStr(product)).append("\"");
        sb.append(",\"quantity\":").append(qty);
        sb.append(",\"destination\":").append(dest);
        sb.append(",\"candidates\":[");
        for (int i = 0; i < results.size(); i++) {
            if (i > 0) sb.append(",");
            OrderAllocator.AllocationResult r = results.get(i);
            sb.append("{\"warehouse_id\":").append(r.warehouseId);
            sb.append(",\"warehouse_name\":\"").append(escStr(r.warehouseName)).append("\"");
            sb.append(",\"city\":\"").append(escStr(r.city)).append("\"");
            sb.append(",\"delivery_cost\":").append(r.deliveryCost);
            sb.append(",\"delivery_time\":").append(r.deliveryTime);
            sb.append(",\"available_stock\":").append(r.availableStock);
            sb.append(",\"can_fulfill\":").append(r.availableStock >= qty);
            sb.append(",\"route\":").append(intListToJson(r.routePath));
            sb.append("}");
        }
        sb.append("]}");
        return sb.toString();
    }

    private static String handleSplit(HttpExchange ex) throws IOException {
        Map<String, Object> body = parseJsonBody(ex);
        String product = (String) body.get("product");
        int qty = toInt(body.get("quantity"));
        int dest = toInt(body.get("destination"));

        OrderSplitter splitter = new OrderSplitter(graph, inventory);
        OrderSplitter.SplitResult sr = splitter.splitOrder(product, qty, dest);

        StringBuilder sb = new StringBuilder();
        sb.append("{\"feasible\":").append(sr.feasible);
        sb.append(",\"total_cost\":").append(sr.totalCost);
        sb.append(",\"max_delivery_time\":").append(sr.maxDeliveryTime);
        sb.append(",\"product\":\"").append(escStr(product)).append("\"");
        sb.append(",\"quantity\":").append(qty);
        sb.append(",\"plan\":[");
        for (int i = 0; i < sr.plan.size(); i++) {
            if (i > 0) sb.append(",");
            OrderSplitter.SplitEntry e = sr.plan.get(i);
            sb.append("{\"warehouse_id\":").append(e.warehouseId);
            sb.append(",\"warehouse_name\":\"").append(escStr(e.warehouseName)).append("\"");
            sb.append(",\"city\":\"").append(escStr(e.city)).append("\"");
            sb.append(",\"units\":").append(e.units);
            sb.append(",\"cost_per_unit\":").append(e.costPerUnit);
            sb.append(",\"total_cost\":").append(e.totalCost);
            sb.append(",\"delivery_time\":").append(e.deliveryTime);
            sb.append(",\"route\":").append(intListToJson(e.routePath));
            sb.append("}");
        }
        sb.append("]}");
        return sb.toString();
    }

    private static String handleDemandRecord(HttpExchange ex) throws IOException {
        Map<String, Object> body = parseJsonBody(ex);
        String product = (String) body.get("product");
        int qty = toInt(body.get("quantity"));
        demandTracker.recordDemand(product, qty);
        return "{\"status\":\"recorded\",\"product\":\"" + escStr(product) + "\",\"quantity\":" + qty + "}";
    }

    private static String handleDemandTrend(HttpExchange ex) {
        String product = getQueryParam(ex, "product", "P001");
        DemandTracker.DemandTrend t = demandTracker.getTrend(product);

        StringBuilder sb = new StringBuilder();
        sb.append("{\"product\":\"").append(escStr(t.productId)).append("\"");
        sb.append(",\"weighted_average\":").append(t.weightedAverage);
        sb.append(",\"total_demand\":").append(t.totalDemand);
        sb.append(",\"window_size\":").append(t.windowSize);
        sb.append(",\"needs_restock\":").append(t.needsRestock);
        sb.append(",\"recent_demand\":").append(intListToJson(t.recentDemand));
        sb.append("}");
        return sb.toString();
    }

    private static String handleDemandAlerts(HttpExchange ex) {
        List<DemandTracker.DemandTrend> alerts = demandTracker.getRestockAlerts();
        StringBuilder sb = new StringBuilder();
        sb.append("{\"alerts\":[");
        for (int i = 0; i < alerts.size(); i++) {
            if (i > 0) sb.append(",");
            DemandTracker.DemandTrend t = alerts.get(i);
            sb.append("{\"product\":\"").append(escStr(t.productId)).append("\"");
            sb.append(",\"weighted_average\":").append(t.weightedAverage);
            sb.append(",\"total_demand\":").append(t.totalDemand);
            sb.append(",\"needs_restock\":").append(t.needsRestock);
            sb.append("}");
        }
        sb.append("]}");
        return sb.toString();
    }

    // ── JSON Helpers ──

    private static String pathResultToJson(Graph.PathResult pr) {
        StringBuilder sb = new StringBuilder();
        sb.append("{\"reachable\":").append(pr.reachable);
        sb.append(",\"total_cost\":").append(pr.totalCost);
        sb.append(",\"total_time\":").append(pr.totalTime);
        sb.append(",\"path\":").append(intListToJson(pr.path));

        // Path details with city names
        sb.append(",\"path_details\":[");
        for (int i = 0; i < pr.path.size(); i++) {
            if (i > 0) sb.append(",");
            int id = pr.path.get(i);
            sb.append("{\"id\":").append(id);
            Graph.Warehouse wh = graph.getWarehouses().get(id);
            if (wh != null) {
                sb.append(",\"name\":\"").append(escStr(wh.name)).append("\"");
                sb.append(",\"city\":\"").append(escStr(wh.city)).append("\"");
            }
            sb.append("}");
        }
        sb.append("]}");
        return sb.toString();
    }

    private static void appendWarehouseArray(StringBuilder sb) {
        sb.append("[");
        boolean first = true;
        for (Graph.Warehouse w : graph.getWarehouses().values()) {
            if (!first) sb.append(",");
            first = false;
            sb.append("{\"id\":").append(w.id);
            sb.append(",\"name\":\"").append(escStr(w.name)).append("\"");
            sb.append(",\"city\":\"").append(escStr(w.city)).append("\"");
            sb.append(",\"lat\":").append(w.lat);
            sb.append(",\"lng\":").append(w.lng);
            sb.append(",\"capacity\":").append(w.capacity);
            sb.append("}");
        }
        sb.append("]");
    }

    private static void appendEdgeArray(StringBuilder sb) {
        sb.append("[");
        Set<String> added = new HashSet<>();
        List<List<Graph.Edge>> adj = graph.getAdj();
        boolean first = true;
        for (int u = 0; u < adj.size(); u++) {
            for (Graph.Edge e : adj.get(u)) {
                int a = Math.min(u, e.to), b = Math.max(u, e.to);
                String key = a + "-" + b;
                if (added.contains(key)) continue;
                added.add(key);
                if (!first) sb.append(",");
                first = false;
                sb.append("{\"from\":").append(u);
                sb.append(",\"to\":").append(e.to);
                sb.append(",\"cost\":").append(e.cost);
                sb.append(",\"time\":").append(e.timeHours);
                sb.append("}");
            }
        }
        sb.append("]");
    }

    private static String intListToJson(List<Integer> list) {
        StringBuilder sb = new StringBuilder("[");
        for (int i = 0; i < list.size(); i++) {
            if (i > 0) sb.append(",");
            sb.append(list.get(i));
        }
        sb.append("]");
        return sb.toString();
    }

    private static String escStr(String s) {
        if (s == null) return "";
        return s.replace("\\", "\\\\").replace("\"", "\\\"")
                .replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t");
    }

    // ── Request Parsing Helpers ──

    private static Map<String, Object> parseJsonBody(HttpExchange ex) throws IOException {
        String body;
        try (InputStream is = ex.getRequestBody()) {
            body = new String(is.readAllBytes(), StandardCharsets.UTF_8);
        }
        return parseSimpleJson(body);
    }

    private static String getQueryParam(HttpExchange ex, String key, String defaultVal) {
        String query = ex.getRequestURI().getQuery();
        if (query == null) return defaultVal;
        for (String param : query.split("&")) {
            String[] kv = param.split("=", 2);
            if (kv.length == 2 && kv[0].equals(key)) return kv[1];
        }
        return defaultVal;
    }

    private static int toInt(Object val) {
        if (val instanceof Number) return ((Number) val).intValue();
        return Integer.parseInt(val.toString());
    }

    /**
     * Minimal JSON object parser — handles flat objects with string/number values.
     * Sufficient for our API request bodies.
     */
    private static Map<String, Object> parseSimpleJson(String json) {
        Map<String, Object> map = new LinkedHashMap<>();
        json = json.trim();
        if (json.startsWith("{")) json = json.substring(1);
        if (json.endsWith("}")) json = json.substring(0, json.length() - 1);

        int i = 0;
        while (i < json.length()) {
            // Skip to key
            int keyStart = json.indexOf('"', i);
            if (keyStart < 0) break;
            int keyEnd = json.indexOf('"', keyStart + 1);
            String key = json.substring(keyStart + 1, keyEnd);

            // Skip to value
            int colon = json.indexOf(':', keyEnd);
            int valStart = colon + 1;
            while (valStart < json.length() && json.charAt(valStart) == ' ') valStart++;

            if (valStart >= json.length()) break;

            char c = json.charAt(valStart);
            if (c == '"') {
                // String value
                int valEnd = json.indexOf('"', valStart + 1);
                map.put(key, json.substring(valStart + 1, valEnd));
                i = valEnd + 1;
            } else {
                // Number value
                int valEnd = valStart;
                while (valEnd < json.length() && json.charAt(valEnd) != ',' && json.charAt(valEnd) != '}')
                    valEnd++;
                String numStr = json.substring(valStart, valEnd).trim();
                if (numStr.contains(".")) {
                    map.put(key, Double.parseDouble(numStr));
                } else {
                    map.put(key, Long.parseLong(numStr));
                }
                i = valEnd;
            }
            // Skip comma
            while (i < json.length() && (json.charAt(i) == ',' || json.charAt(i) == ' ')) i++;
        }
        return map;
    }
}
