#include "fenwick.h"
#include <fstream>
#include <sstream>
#include <algorithm>

// ──────────────────────────────────────────────
// Fenwick Tree (BIT) Implementation
//   Index 1-based internally for clean bit manipulation
//
//   Key insight: i & (-i) extracts the lowest set bit,
//   which determines the range each node covers.
// ──────────────────────────────────────────────

static std::string trimStr(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

FenwickTree::FenwickTree(int n) : n_(n), tree_(n + 1, 0) {}

// Point update: add delta to position i
// Propagates up using i += i & (-i)
void FenwickTree::update(int i, int delta) {
    for (; i <= n_; i += i & (-i))
        tree_[i] += delta;
}

// Prefix sum query: returns sum of [1..i]
// Walks down using i -= i & (-i)
int FenwickTree::query(int i) const {
    int sum = 0;
    for (; i > 0; i -= i & (-i))
        sum += tree_[i];
    return sum;
}

// Range sum query: sum of [l..r]
int FenwickTree::rangeQuery(int l, int r) const {
    if (l > r) return 0;
    return query(r) - query(l - 1);
}

// Point query: value at position i
int FenwickTree::pointQuery(int i) const {
    return rangeQuery(i, i);
}

// ──────────────────────────────────────────────
// Inventory Manager
// ──────────────────────────────────────────────

void InventoryManager::loadInventory(const std::string& path, int warehouseCount) {
    whCount_ = warehouseCount;

    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // skip header

    // First pass: collect all data
    struct Row {
        int whId;
        std::string prodId, prodName;
        int stock, price;
    };
    std::vector<Row> rows;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        Row r;

        std::getline(ss, token, ','); r.whId = std::stoi(trimStr(token));
        std::getline(ss, token, ','); r.prodId = trimStr(token);
        std::getline(ss, token, ','); r.prodName = trimStr(token);
        std::getline(ss, token, ','); r.stock = std::stoi(trimStr(token));
        std::getline(ss, token, ','); r.price = std::stoi(trimStr(token));

        rows.push_back(r);

        // Register product
        if (products_.find(r.prodId) == products_.end()) {
            products_[r.prodId] = {r.prodId, r.prodName, r.price};
        }
    }

    // Initialize Fenwick Trees and raw stock arrays
    for (auto& pair : products_) {
        const std::string& pid = pair.first;
        trees_[pid] = FenwickTree(warehouseCount);
        rawStock_[pid].assign(warehouseCount + 1, 0);
    }

    // Populate with stock data
    for (auto& r : rows) {
        if (r.whId >= 1 && r.whId <= warehouseCount) {
            trees_[r.prodId].update(r.whId, r.stock);
            rawStock_[r.prodId][r.whId] = r.stock;
        }
    }
}

int InventoryManager::getStock(const std::string& productId, int warehouseId) const {
    auto it = rawStock_.find(productId);
    if (it == rawStock_.end()) return 0;
    if (warehouseId < 1 || warehouseId >= (int)it->second.size()) return 0;
    return it->second[warehouseId];
}

int InventoryManager::getRegionStock(const std::string& productId, int fromWh, int toWh) const {
    auto it = trees_.find(productId);
    if (it == trees_.end()) return 0;
    return it->second.rangeQuery(fromWh, toWh);
}

int InventoryManager::getTotalStock(const std::string& productId) const {
    auto it = trees_.find(productId);
    if (it == trees_.end()) return 0;
    return it->second.query(whCount_);
}

bool InventoryManager::updateStock(const std::string& productId, int warehouseId, int delta) {
    auto it = rawStock_.find(productId);
    if (it == rawStock_.end()) return false;
    if (warehouseId < 1 || warehouseId >= (int)it->second.size()) return false;

    int current = it->second[warehouseId];
    if (current + delta < 0) return false; // can't go negative

    it->second[warehouseId] += delta;
    trees_[productId].update(warehouseId, delta);
    return true;
}

std::vector<std::pair<int, int>> InventoryManager::getWarehousesWithStock(
    const std::string& productId, int minQty) const 
{
    std::vector<std::pair<int, int>> result;
    auto it = rawStock_.find(productId);
    if (it == rawStock_.end()) return result;

    for (int i = 1; i <= whCount_; i++) {
        int stock = it->second[i];
        if (stock >= minQty) {
            result.push_back({i, stock});
        }
    }
    return result;
}
