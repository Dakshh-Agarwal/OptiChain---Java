// ──────────────────────────────────────────────────────────────
// DSA Engine — Main Entry Point
//
// Protocol: Reads JSON commands from stdin, writes JSON to stdout.
// Each line of stdin is one JSON command. Engine processes it and
// writes one line of JSON response.
//
// Commands:
//   { "cmd": "init", "dataDir": "..." }
//   { "cmd": "dijkstra", "from": 1, "to": 5 }
//   { "cmd": "bfs", "from": 1, "to": 5 }
//   { "cmd": "warehouses" }
//   { "cmd": "inventory", "product": "P001" }
//   { "cmd": "all_inventory" }
//   { "cmd": "allocate", "product": "P001", "qty": 50, "dest": 1 }
//   { "cmd": "split", "product": "P001", "qty": 200, "dest": 1 }
//   { "cmd": "demand_record", "product": "P001", "qty": 30 }
//   { "cmd": "demand_trend", "product": "P001" }
//   { "cmd": "demand_alerts" }
//   { "cmd": "graph_data" }
//   { "cmd": "quit" }
// ──────────────────────────────────────────────────────────────

#include <iostream>
#include <string>
#include <sstream>
#include <set>

#include "json.hpp"
#include "graph.h"
#include "fenwick.h"
#include "allocator.h"
#include "splitter.h"
#include "demand.h"

Graph graph;
InventoryManager inventory;
DemandTracker demandTracker(7, 50.0);
bool initialized = false;

// ──────────────────────────────────────────────
// Command Handlers
// ──────────────────────────────────────────────

json::Value handleInit(const json::Value& cmd) {
    std::string dataDir = cmd["dataDir"].asString();
    
    graph.loadWarehouses(dataDir + "/warehouses.csv");
    graph.loadRoutes(dataDir + "/routes.csv");
    inventory.loadInventory(dataDir + "/inventory.csv", 25);
    
    initialized = true;
    
    json::Value resp;
    resp["status"] = "ok";
    resp["warehouses"] = (int64_t)graph.getWarehouses().size();
    resp["products"] = (int64_t)inventory.getProducts().size();
    return resp;
}

json::Value handleDijkstra(const json::Value& cmd) {
    int from = (int)cmd["from"].asInt();
    int to = (int)cmd["to"].asInt();
    
    PathResult pr = graph.dijkstra(from, to);
    
    json::Value resp;
    resp["reachable"] = pr.reachable;
    resp["total_cost"] = pr.total_cost;
    resp["total_time"] = pr.total_time;
    
    json::Array pathArr;
    for (int id : pr.path) pathArr.push_back(json::Value((int64_t)id));
    resp["path"] = pathArr;
    
    // Add path details with city names
    json::Array pathDetails;
    for (int id : pr.path) {
        json::Value node;
        node["id"] = (int64_t)id;
        auto& wh = graph.getWarehouses();
        auto it = wh.find(id);
        if (it != wh.end()) {
            node["name"] = it->second.name;
            node["city"] = it->second.city;
        }
        pathDetails.push_back(node);
    }
    resp["path_details"] = pathDetails;
    
    return resp;
}

json::Value handleBFS(const json::Value& cmd) {
    int from = (int)cmd["from"].asInt();
    int to = (int)cmd["to"].asInt();
    
    PathResult pr = graph.bfs(from, to);
    
    json::Value resp;
    resp["reachable"] = pr.reachable;
    resp["total_cost"] = pr.total_cost;
    resp["total_time"] = pr.total_time;
    resp["hops"] = (int64_t)(pr.path.size() > 0 ? pr.path.size() - 1 : 0);
    
    json::Array pathArr;
    for (int id : pr.path) pathArr.push_back(json::Value((int64_t)id));
    resp["path"] = pathArr;
    
    return resp;
}

json::Value handleWarehouses() {
    json::Array arr;
    for (auto& pair : graph.getWarehouses()) {
        int id = pair.first;
        const Warehouse& w = pair.second;
        json::Value node;
        node["id"] = (int64_t)w.id;
        node["name"] = w.name;
        node["city"] = w.city;
        node["lat"] = w.lat;
        node["lng"] = w.lng;
        node["capacity"] = (int64_t)w.capacity;
        arr.push_back(node);
    }
    json::Value resp;
    resp["warehouses"] = arr;
    return resp;
}

json::Value handleInventory(const json::Value& cmd) {
    std::string productId = cmd["product"].asString();
    
    json::Value resp;
    resp["product"] = productId;
    resp["total_stock"] = (int64_t)inventory.getTotalStock(productId);
    
    json::Array stocks;
    for (int i = 1; i <= inventory.warehouseCount(); i++) {
        int stock = inventory.getStock(productId, i);
        if (stock > 0) {
            json::Value entry;
            entry["warehouse_id"] = (int64_t)i;
            entry["stock"] = (int64_t)stock;
            
            auto& wh = graph.getWarehouses();
            auto it = wh.find(i);
            if (it != wh.end()) {
                entry["city"] = it->second.city;
                entry["name"] = it->second.name;
            }
            stocks.push_back(entry);
        }
    }
    resp["warehouses"] = stocks;
    
    return resp;
}

json::Value handleAllInventory() {
    json::Value resp;
    json::Array products;
    
    for (auto& pair : inventory.getProducts()) {
        const std::string& pid = pair.first;
        const ProductInfo& info = pair.second;
        json::Value prod;
        prod["id"] = info.id;
        prod["name"] = info.name;
        prod["unit_price"] = (int64_t)info.unitPrice;
        prod["total_stock"] = (int64_t)inventory.getTotalStock(pid);
        
        json::Array stocks;
        for (int i = 1; i <= inventory.warehouseCount(); i++) {
            int stock = inventory.getStock(pid, i);
            json::Value entry;
            entry["warehouse_id"] = (int64_t)i;
            entry["stock"] = (int64_t)stock;
            stocks.push_back(entry);
        }
        prod["warehouses"] = stocks;
        products.push_back(prod);
    }
    
    resp["products"] = products;
    return resp;
}

json::Value handleAllocate(const json::Value& cmd) {
    std::string product = cmd["product"].asString();
    int qty = (int)cmd["qty"].asInt();
    int dest = (int)cmd["dest"].asInt();
    
    OrderAllocator allocator(graph, inventory);
    auto results = allocator.findBestWarehouses(product, qty, dest, 5);
    
    json::Value resp;
    json::Array arr;
    
    for (auto& r : results) {
        json::Value entry;
        entry["warehouse_id"] = (int64_t)r.warehouseId;
        entry["warehouse_name"] = r.warehouseName;
        entry["city"] = r.city;
        entry["delivery_cost"] = r.deliveryCost;
        entry["delivery_time"] = r.deliveryTime;
        entry["available_stock"] = (int64_t)r.availableStock;
        entry["can_fulfill"] = r.availableStock >= qty;
        
        json::Array path;
        for (int id : r.routePath) path.push_back(json::Value((int64_t)id));
        entry["route"] = path;
        
        arr.push_back(entry);
    }
    
    resp["candidates"] = arr;
    resp["product"] = product;
    resp["quantity"] = (int64_t)qty;
    resp["destination"] = (int64_t)dest;
    
    return resp;
}

json::Value handleSplit(const json::Value& cmd) {
    std::string product = cmd["product"].asString();
    int qty = (int)cmd["qty"].asInt();
    int dest = (int)cmd["dest"].asInt();
    
    OrderSplitter splitter(graph, inventory);
    SplitResult sr = splitter.splitOrder(product, qty, dest);
    
    json::Value resp;
    resp["feasible"] = sr.feasible;
    resp["total_cost"] = sr.totalCost;
    resp["max_delivery_time"] = sr.maxDeliveryTime;
    resp["product"] = product;
    resp["quantity"] = (int64_t)qty;
    
    json::Array plan;
    for (auto& entry : sr.plan) {
        json::Value e;
        e["warehouse_id"] = (int64_t)entry.warehouseId;
        e["warehouse_name"] = entry.warehouseName;
        e["city"] = entry.city;
        e["units"] = (int64_t)entry.units;
        e["cost_per_unit"] = entry.costPerUnit;
        e["total_cost"] = entry.totalCost;
        e["delivery_time"] = entry.deliveryTime;
        
        json::Array path;
        for (int id : entry.routePath) path.push_back(json::Value((int64_t)id));
        e["route"] = path;
        
        plan.push_back(e);
    }
    resp["plan"] = plan;
    
    return resp;
}

json::Value handleDemandRecord(const json::Value& cmd) {
    std::string product = cmd["product"].asString();
    int qty = (int)cmd["qty"].asInt();
    
    demandTracker.recordDemand(product, qty);
    
    json::Value resp;
    resp["status"] = "recorded";
    resp["product"] = product;
    resp["quantity"] = (int64_t)qty;
    return resp;
}

json::Value handleDemandTrend(const json::Value& cmd) {
    std::string product = cmd["product"].asString();
    
    DemandTrend trend = demandTracker.getTrend(product);
    
    json::Value resp;
    resp["product"] = trend.productId;
    resp["weighted_average"] = trend.weightedAverage;
    resp["total_demand"] = trend.totalDemand;
    resp["window_size"] = (int64_t)trend.windowSize;
    resp["needs_restock"] = trend.needsRestock;
    
    json::Array recent;
    for (int v : trend.recentDemand) recent.push_back(json::Value((int64_t)v));
    resp["recent_demand"] = recent;
    
    return resp;
}

json::Value handleDemandAlerts() {
    auto alerts = demandTracker.getRestockAlerts();
    
    json::Value resp;
    json::Array arr;
    for (auto& t : alerts) {
        json::Value a;
        a["product"] = t.productId;
        a["weighted_average"] = t.weightedAverage;
        a["total_demand"] = t.totalDemand;
        a["needs_restock"] = t.needsRestock;
        arr.push_back(a);
    }
    resp["alerts"] = arr;
    return resp;
}

json::Value handleGraphData() {
    json::Value resp;
    
    // Nodes
    json::Array nodes;
    for (auto& pair : graph.getWarehouses()) {
        int id = pair.first;
        const Warehouse& w = pair.second;
        json::Value node;
        node["id"] = (int64_t)w.id;
        node["name"] = w.name;
        node["city"] = w.city;
        node["lat"] = w.lat;
        node["lng"] = w.lng;
        node["capacity"] = (int64_t)w.capacity;
        nodes.push_back(node);
    }
    resp["nodes"] = nodes;
    
    // Edges (deduplicated)
    json::Array edges;
    auto& adj = graph.getAdj();
    // Track added edges to avoid duplicates (bidirectional)
    std::set<std::pair<int,int>> added;
    for (int u = 0; u < (int)adj.size(); u++) {
        for (auto& e : adj[u]) {
            int a = std::min(u, e.to);
            int b = std::max(u, e.to);
            if (added.count({a, b})) continue;
            added.insert({a, b});
            
            json::Value edge;
            edge["from"] = (int64_t)u;
            edge["to"] = (int64_t)e.to;
            edge["cost"] = e.cost;
            edge["time"] = e.time_hours;
            edges.push_back(edge);
        }
    }
    resp["edges"] = edges;
    
    return resp;
}

// ──────────────────────────────────────────────
// Main Loop — reads JSON commands from stdin
// ──────────────────────────────────────────────
int main() {
    // Disable sync for faster I/O
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        
        try {
            json::Value cmd = json::parse(line);
            std::string command = cmd["cmd"].asString();
            
            json::Value response;
            
            if (command == "init") {
                response = handleInit(cmd);
            } else if (command == "quit") {
                json::Value quit;
                quit["status"] = "bye";
                std::cout << quit.dump() << "\n";
                std::cout.flush();
                break;
            } else if (!initialized) {
                response["error"] = "Engine not initialized. Send init command first.";
            } else if (command == "dijkstra") {
                response = handleDijkstra(cmd);
            } else if (command == "bfs") {
                response = handleBFS(cmd);
            } else if (command == "warehouses") {
                response = handleWarehouses();
            } else if (command == "inventory") {
                response = handleInventory(cmd);
            } else if (command == "all_inventory") {
                response = handleAllInventory();
            } else if (command == "allocate") {
                response = handleAllocate(cmd);
            } else if (command == "split") {
                response = handleSplit(cmd);
            } else if (command == "demand_record") {
                response = handleDemandRecord(cmd);
            } else if (command == "demand_trend") {
                response = handleDemandTrend(cmd);
            } else if (command == "demand_alerts") {
                response = handleDemandAlerts();
            } else if (command == "graph_data") {
                response = handleGraphData();
            } else {
                response["error"] = "Unknown command: " + command;
            }
            
            std::cout << response.dump() << "\n";
            std::cout.flush();
            
        } catch (const std::exception& e) {
            json::Value err;
            err["error"] = std::string("Exception: ") + e.what();
            std::cout << err.dump() << "\n";
            std::cout.flush();
        }
    }
    
    return 0;
}
