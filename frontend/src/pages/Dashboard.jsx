import { useState, useEffect } from 'react';
import { api } from '../api';

export default function Dashboard() {
  const [graph, setGraph] = useState(null);
  const [inventory, setInventory] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadData();
  }, []);

  async function loadData() {
    try {
      const [g, inv] = await Promise.all([
        api.getGraph(),
        api.getAllInventory(),
      ]);
      setGraph(g);
      setInventory(inv);
    } catch (err) {
      console.error('Failed to load dashboard:', err);
    } finally {
      setLoading(false);
    }
  }

  if (loading) {
    return (
      <div className="loading-overlay">
        <div className="spinner"></div>
        Loading system data...
      </div>
    );
  }

  const nodeCount = graph?.nodes?.length || 0;
  const edgeCount = graph?.edges?.length || 0;
  const productCount = inventory?.products?.length || 0;
  
  const totalStock = inventory?.products?.reduce((sum, p) => sum + (p.total_stock || 0), 0) || 0;

  return (
    <div className="fade-in">
      {/* Stats */}
      <div className="stats-grid">
        <div className="stat-card">
          <div className="stat-icon indigo">🏭</div>
          <div className="stat-info">
            <div className="stat-value">{nodeCount}</div>
            <div className="stat-label">Warehouse Nodes</div>
          </div>
        </div>
        <div className="stat-card">
          <div className="stat-icon cyan">🔗</div>
          <div className="stat-info">
            <div className="stat-value">{edgeCount}</div>
            <div className="stat-label">Route Edges</div>
          </div>
        </div>
        <div className="stat-card">
          <div className="stat-icon emerald">📦</div>
          <div className="stat-info">
            <div className="stat-value">{productCount}</div>
            <div className="stat-label">Product SKUs</div>
          </div>
        </div>
        <div className="stat-card">
          <div className="stat-icon amber">📊</div>
          <div className="stat-info">
            <div className="stat-value">{totalStock.toLocaleString()}</div>
            <div className="stat-label">Total Units in Stock</div>
          </div>
        </div>
      </div>

      {/* DSA Module Cards */}
      <div className="grid-2" style={{ marginBottom: '20px' }}>
        <div className="card">
          <div className="card-title">
            <span className="card-icon">◈</span>
            Logistics Graph Engine
          </div>
          <div className="algo-tags">
            <span className="algo-tag">Dijkstra</span>
            <span className="algo-tag">BFS</span>
            <span className="algo-tag">Adjacency List</span>
            <span className="algo-tag">Min Heap</span>
          </div>
          <p style={{ fontSize: '13px', color: 'var(--text-secondary)', lineHeight: 1.6, marginBottom: '12px' }}>
            Models {nodeCount} warehouse nodes with {edgeCount} weighted edges. Finds optimal delivery routes using Dijkstra's shortest path algorithm.
          </p>
          <div className="complexity-box">
            Time: O((V+E) log V) &nbsp;|&nbsp; Space: O(V + E)
          </div>
        </div>

        <div className="card">
          <div className="card-title">
            <span className="card-icon">▦</span>
            Inventory Engine
          </div>
          <div className="algo-tags">
            <span className="algo-tag">Fenwick Tree</span>
            <span className="algo-tag">BIT</span>
            <span className="algo-tag">Range Query</span>
            <span className="algo-tag">Point Update</span>
          </div>
          <p style={{ fontSize: '13px', color: 'var(--text-secondary)', lineHeight: 1.6, marginBottom: '12px' }}>
            Tracks {totalStock.toLocaleString()} units across {nodeCount} warehouses using Binary Indexed Trees. Supports O(log n) stock updates and range sum queries.
          </p>
          <div className="complexity-box">
            Update: O(log n) &nbsp;|&nbsp; Range Query: O(log n)
          </div>
        </div>
      </div>

      <div className="grid-2">
        <div className="card">
          <div className="card-title">
            <span className="card-icon">⚡</span>
            Order Splitting Engine
          </div>
          <div className="algo-tags">
            <span className="algo-tag">Partition DP</span>
            <span className="algo-tag">Bounded Knapsack</span>
            <span className="algo-tag">Backtracking</span>
          </div>
          <p style={{ fontSize: '13px', color: 'var(--text-secondary)', lineHeight: 1.6, marginBottom: '12px' }}>
            When no single warehouse can fulfill an order, splits optimally across multiple warehouses using dynamic programming to minimize total cost.
          </p>
          <div className="complexity-box">
            Time: O(U × W × K) &nbsp;|&nbsp; Space: O(U)
          </div>
        </div>

        <div className="card">
          <div className="card-title">
            <span className="card-icon">◉</span>
            Demand Tracker
          </div>
          <div className="algo-tags">
            <span className="algo-tag">Sliding Window</span>
            <span className="algo-tag">Deque</span>
            <span className="algo-tag">Weighted Average</span>
            <span className="algo-tag">HashMap</span>
          </div>
          <p style={{ fontSize: '13px', color: 'var(--text-secondary)', lineHeight: 1.6, marginBottom: '12px' }}>
            Tracks demand patterns per product using a sliding window deque. Computes weighted moving averages with recent data weighted more heavily.
          </p>
          <div className="complexity-box">
            Record: O(1) amortized &nbsp;|&nbsp; Trend: O(W)
          </div>
        </div>
      </div>
    </div>
  );
}
