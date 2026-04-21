#pragma once
#include <vector>
#include <string>
#include <unordered_map>

// ──────────────────────────────────────────────
// Fenwick Tree (Binary Indexed Tree)
//   Supports O(log n) point updates and prefix queries
//   Used for inventory tracking across warehouses
//
//   Operations:
//     update(i, delta)    → O(log n)
//     query(i)            → O(log n) prefix sum [1..i]
//     rangeQuery(l, r)    → O(log n) range sum [l..r]
// ──────────────────────────────────────────────
class FenwickTree {
public:
    FenwickTree() = default;
    explicit FenwickTree(int n);

    void update(int i, int delta);
    int query(int i) const;           // prefix sum [1..i]
    int rangeQuery(int l, int r) const; // sum [l..r]
    int pointQuery(int i) const;       // value at position i
    int size() const { return n_; }

private:
    int n_ = 0;
    std::vector<int> tree_;
};

// ──────────────────────────────────────────────
// Inventory Manager
//   Manages one FenwickTree per product
//   Loads initial data from CSV
// ──────────────────────────────────────────────
struct ProductInfo {
    std::string id;
    std::string name;
    int unitPrice;
};

class InventoryManager {
public:
    InventoryManager() = default;

    void loadInventory(const std::string& path, int warehouseCount);

    // Stock operations
    int getStock(const std::string& productId, int warehouseId) const;
    int getRegionStock(const std::string& productId, int fromWh, int toWh) const;
    int getTotalStock(const std::string& productId) const;
    bool updateStock(const std::string& productId, int warehouseId, int delta);

    // Get warehouses with enough stock for a product
    std::vector<std::pair<int, int>> getWarehousesWithStock(const std::string& productId, int minQty) const;

    // Product catalog
    const std::unordered_map<std::string, ProductInfo>& getProducts() const { return products_; }
    const std::unordered_map<std::string, FenwickTree>& getTrees() const { return trees_; }
    
    // Raw stock for direct access (used for DP/allocation)
    int warehouseCount() const { return whCount_; }

private:
    int whCount_ = 0;
    std::unordered_map<std::string, FenwickTree> trees_;       // productId -> Fenwick Tree
    std::unordered_map<std::string, std::vector<int>> rawStock_; // productId -> per-warehouse stock
    std::unordered_map<std::string, ProductInfo> products_;
};
