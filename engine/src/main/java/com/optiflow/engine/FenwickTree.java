package com.optiflow.engine;

/**
 * ──────────────────────────────────────────────
 * Fenwick Tree (Binary Indexed Tree)
 *   Supports O(log n) point updates and prefix queries
 *   Used for inventory tracking across warehouses
 *
 *   Operations:
 *     update(i, delta)    → O(log n)
 *     query(i)            → O(log n) prefix sum [1..i]
 *     rangeQuery(l, r)    → O(log n) range sum [l..r]
 *
 *   Key insight: i & (-i) extracts the lowest set bit,
 *   which determines the range each node covers.
 * ──────────────────────────────────────────────
 */
public class FenwickTree {

    private int n;
    private int[] tree;

    public FenwickTree(int n) {
        this.n = n;
        this.tree = new int[n + 1];
    }

    /**
     * Point update: add delta to position i
     * Propagates up using i += i & (-i)
     */
    public void update(int i, int delta) {
        for (; i <= n; i += i & (-i))
            tree[i] += delta;
    }

    /**
     * Prefix sum query: returns sum of [1..i]
     * Walks down using i -= i & (-i)
     */
    public int query(int i) {
        int sum = 0;
        for (; i > 0; i -= i & (-i))
            sum += tree[i];
        return sum;
    }

    /**
     * Range sum query: sum of [l..r]
     */
    public int rangeQuery(int l, int r) {
        if (l > r) return 0;
        return query(r) - query(l - 1);
    }

    /**
     * Point query: value at position i
     */
    public int pointQuery(int i) {
        return rangeQuery(i, i);
    }

    public int size() {
        return n;
    }
}
