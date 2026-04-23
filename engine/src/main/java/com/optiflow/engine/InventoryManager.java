package com.optiflow.engine;

import java.io.*;
import java.util.*;

/**
 * ──────────────────────────────────────────────
 * Inventory Manager
 *   Manages one FenwickTree per product
 *   Loads initial data from CSV
 * ──────────────────────────────────────────────
 */
public class InventoryManager {

    public static class ProductInfo {
        public String id;
        public String name;
        public int unitPrice;

        public ProductInfo(String id, String name, int unitPrice) {
            this.id = id;
            this.name = name;
            this.unitPrice = unitPrice;
        }
    }

    private int whCount = 0;
    private final Map<String, FenwickTree> trees = new LinkedHashMap<>();
    private final Map<String, int[]> rawStock = new HashMap<>();
    private final Map<String, ProductInfo> products = new LinkedHashMap<>();

    private static String trim(String s) {
        return s == null ? "" : s.trim();
    }

    public void loadInventory(String path, int warehouseCount) throws IOException {
        this.whCount = warehouseCount;

        // First pass: collect all data
        List<int[]> stockRows = new ArrayList<>();    // [whId, stock, price]
        List<String[]> infoRows = new ArrayList<>();  // [prodId, prodName]

        try (BufferedReader br = new BufferedReader(new FileReader(path))) {
            br.readLine(); // skip header
            String line;
            while ((line = br.readLine()) != null) {
                String[] tokens = line.split(",");
                int whId = Integer.parseInt(trim(tokens[0]));
                String prodId = trim(tokens[1]);
                String prodName = trim(tokens[2]);
                int stock = Integer.parseInt(trim(tokens[3]));
                int price = Integer.parseInt(trim(tokens[4]));

                stockRows.add(new int[]{whId, stock, price});
                infoRows.add(new String[]{prodId, prodName});

                // Register product
                if (!products.containsKey(prodId)) {
                    products.put(prodId, new ProductInfo(prodId, prodName, price));
                }
            }
        }

        // Initialize Fenwick Trees and raw stock arrays
        for (String pid : products.keySet()) {
            trees.put(pid, new FenwickTree(warehouseCount));
            rawStock.put(pid, new int[warehouseCount + 1]);
        }

        // Populate with stock data
        for (int i = 0; i < stockRows.size(); i++) {
            int whId = stockRows.get(i)[0];
            int stock = stockRows.get(i)[1];
            String prodId = infoRows.get(i)[0];

            if (whId >= 1 && whId <= warehouseCount) {
                trees.get(prodId).update(whId, stock);
                rawStock.get(prodId)[whId] = stock;
            }
        }
    }

    public int getStock(String productId, int warehouseId) {
        int[] stocks = rawStock.get(productId);
        if (stocks == null) return 0;
        if (warehouseId < 1 || warehouseId >= stocks.length) return 0;
        return stocks[warehouseId];
    }

    public int getRegionStock(String productId, int fromWh, int toWh) {
        FenwickTree ft = trees.get(productId);
        if (ft == null) return 0;
        return ft.rangeQuery(fromWh, toWh);
    }

    public int getTotalStock(String productId) {
        FenwickTree ft = trees.get(productId);
        if (ft == null) return 0;
        return ft.query(whCount);
    }

    public boolean updateStock(String productId, int warehouseId, int delta) {
        int[] stocks = rawStock.get(productId);
        if (stocks == null) return false;
        if (warehouseId < 1 || warehouseId >= stocks.length) return false;

        int current = stocks[warehouseId];
        if (current + delta < 0) return false; // can't go negative

        stocks[warehouseId] += delta;
        trees.get(productId).update(warehouseId, delta);
        return true;
    }

    /**
     * Get warehouses with enough stock for a product.
     * Returns list of [warehouseId, stock] pairs.
     */
    public List<int[]> getWarehousesWithStock(String productId, int minQty) {
        List<int[]> result = new ArrayList<>();
        int[] stocks = rawStock.get(productId);
        if (stocks == null) return result;

        for (int i = 1; i <= whCount; i++) {
            if (stocks[i] >= minQty) {
                result.add(new int[]{i, stocks[i]});
            }
        }
        return result;
    }

    public Map<String, ProductInfo> getProducts() {
        return products;
    }

    public Map<String, FenwickTree> getTrees() {
        return trees;
    }

    public int warehouseCount() {
        return whCount;
    }
}
