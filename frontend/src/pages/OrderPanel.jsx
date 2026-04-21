import { useState, useEffect } from 'react';
import { api } from '../api';

export default function OrderPanel() {
  const [warehouses, setWarehouses] = useState([]);
  const [products, setProducts] = useState([]);
  const [productId, setProductId] = useState('P001');
  const [quantity, setQuantity] = useState(50);
  const [destination, setDestination] = useState(1);
  const [mode, setMode] = useState('allocate'); // 'allocate' or 'split'
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    loadData();
  }, []);

  async function loadData() {
    try {
      const [wh, inv] = await Promise.all([
        api.getWarehouses(),
        api.getAllInventory(),
      ]);
      setWarehouses((wh.warehouses || []).sort((a, b) => a.id - b.id));
      setProducts(inv.products || []);
    } catch (err) {
      console.error('Failed to load data:', err);
    }
  }

  async function runOrder() {
    setLoading(true);
    setResult(null);
    try {
      let res;
      if (mode === 'allocate') {
        res = await api.allocate(productId, quantity, destination);
        res._mode = 'allocate';
      } else {
        res = await api.split(productId, quantity, destination);
        res._mode = 'split';
      }
      setResult(res);

      // Also record demand
      await api.recordDemand(productId, quantity);
    } catch (err) {
      console.error('Order failed:', err);
    } finally {
      setLoading(false);
    }
  }

  const selectedProduct = products.find(p => p.id === productId);

  return (
    <div className="fade-in">
      <div className="algo-tags">
        <span className="algo-tag">Min Heap</span>
        <span className="algo-tag">Dijkstra Routing</span>
        <span className="algo-tag">Fenwick Tree Query</span>
        <span className="algo-tag">Partition DP</span>
        <span className="algo-tag">Bounded Knapsack</span>
      </div>

      <div className="grid-1-2">
        {/* Order Form */}
        <div>
          <div className="card" style={{ marginBottom: '16px' }}>
            <div className="card-title">
              <span className="card-icon">📋</span>
              Place Order
            </div>

            <div className="form-group">
              <label className="form-label">Mode</label>
              <select className="form-select" value={mode} onChange={e => setMode(e.target.value)}>
                <option value="allocate">Single Warehouse (Min-Heap)</option>
                <option value="split">Multi-Warehouse Split (DP)</option>
              </select>
            </div>

            <div className="form-group">
              <label className="form-label">Product</label>
              <select className="form-select" value={productId} onChange={e => setProductId(e.target.value)}>
                {products.map(p => (
                  <option key={p.id} value={p.id}>{p.id} — {p.name}</option>
                ))}
              </select>
            </div>

            {selectedProduct && (
              <div style={{ 
                padding: '10px 14px', 
                background: 'rgba(0,0,0,0.2)', 
                borderRadius: '8px', 
                marginBottom: '16px',
                fontSize: '12px',
                color: 'var(--text-secondary)'
              }}>
                <div>Total stock: <strong style={{ color: 'var(--accent-emerald)' }}>{selectedProduct.total_stock}</strong> units</div>
                <div>Price: <strong>₹{selectedProduct.unit_price?.toLocaleString()}</strong></div>
              </div>
            )}

            <div className="form-group">
              <label className="form-label">Quantity</label>
              <input 
                type="number" 
                className="form-input" 
                value={quantity} 
                onChange={e => setQuantity(Number(e.target.value))}
                min={1}
              />
            </div>

            <div className="form-group">
              <label className="form-label">Deliver To (Warehouse Hub)</label>
              <select className="form-select" value={destination} onChange={e => setDestination(Number(e.target.value))}>
                {warehouses.map(w => (
                  <option key={w.id} value={w.id}>{w.id} — {w.city}</option>
                ))}
              </select>
            </div>

            <button className="btn btn-success btn-block" onClick={runOrder} disabled={loading}>
              {loading ? <><div className="spinner"></div> Processing...</> : '⚡ Execute Order'}
            </button>
          </div>

          {/* How it works */}
          <div className="card">
            <div className="card-title">
              <span className="card-icon">🧠</span>
              How It Works
            </div>
            <div style={{ fontSize: '12px', color: 'var(--text-secondary)', lineHeight: 1.8 }}>
              <p><strong style={{ color: 'var(--accent-indigo)' }}>Single Warehouse:</strong></p>
              <p>1. Fenwick Tree → find warehouses with stock</p>
              <p>2. Dijkstra → compute delivery cost for each</p>
              <p>3. Min-Heap → rank by cost, return cheapest</p>
              <br />
              <p><strong style={{ color: 'var(--accent-violet)' }}>Multi-Warehouse Split:</strong></p>
              <p>1. Identify all candidates with partial stock</p>
              <p>2. Partition DP → find optimal split</p>
              <p>3. dp[i] = min cost to fulfill i units</p>
              <p>4. Backtrack → which warehouses, how many units</p>
            </div>
          </div>
        </div>

        {/* Result */}
        <div>
          {!result && (
            <div className="card">
              <div className="empty-state">
                <div className="empty-icon">⚡</div>
                <h3>No Order Placed Yet</h3>
                <p>Configure and execute an order to see the fulfillment plan</p>
              </div>
            </div>
          )}

          {result && result._mode === 'allocate' && (
            <div className="card">
              <div className="card-title">
                <span className="card-icon">🏆</span>
                Allocation Result — Top Candidates
              </div>

              <div style={{ marginBottom: '12px' }}>
                <span className="badge badge-info">
                  {result.candidates?.length || 0} candidates evaluated
                </span>
              </div>

              {(result.candidates || []).map((c, i) => (
                <div key={i} className="split-card">
                  <div className="split-info">
                    <div className="split-wh">
                      {i === 0 ? '🥇' : i === 1 ? '🥈' : i === 2 ? '🥉' : '•'} {c.warehouse_name}
                    </div>
                    <div className="split-city">{c.city} · Stock: {c.available_stock} units</div>
                    <div style={{ marginTop: '4px' }}>
                      {c.can_fulfill 
                        ? <span className="badge badge-success">Can Fulfill</span>
                        : <span className="badge badge-warning">Partial Only</span>
                      }
                    </div>
                  </div>
                  <div style={{ textAlign: 'right' }}>
                    <div className="split-qty">₹{c.delivery_cost?.toLocaleString()}</div>
                    <div className="split-units">{c.delivery_time}h transit</div>
                  </div>
                </div>
              ))}

              {result.candidates?.length > 0 && (
                <div className="result-panel" style={{ marginTop: '12px' }}>
                  <div className="result-header">✅ Recommended: {result.candidates[0].warehouse_name}</div>
                  <div className="result-row">
                    <span className="label">Delivery Cost</span>
                    <span className="value" style={{ color: 'var(--accent-emerald)' }}>
                      ₹{result.candidates[0].delivery_cost?.toLocaleString()}
                    </span>
                  </div>
                  <div className="result-row">
                    <span className="label">ETA</span>
                    <span className="value">{result.candidates[0].delivery_time}h</span>
                  </div>
                  <div className="result-row">
                    <span className="label">Route</span>
                    <span className="value">{result.candidates[0].route?.join(' → ')}</span>
                  </div>
                </div>
              )}
            </div>
          )}

          {result && result._mode === 'split' && (
            <div className="card">
              <div className="card-title">
                <span className="card-icon">🔀</span>
                Split Fulfillment Plan
              </div>

              {result.feasible ? (
                <>
                  <div style={{ display: 'flex', gap: '8px', marginBottom: '16px' }}>
                    <span className="badge badge-success">Feasible</span>
                    <span className="badge badge-info">{result.plan?.length || 0} warehouses</span>
                  </div>

                  {(result.plan || []).map((entry, i) => (
                    <div key={i} className="split-card">
                      <div className="split-info">
                        <div className="split-wh">{entry.warehouse_name}</div>
                        <div className="split-city">{entry.city} · Route: {entry.route?.join('→')}</div>
                      </div>
                      <div style={{ textAlign: 'right' }}>
                        <div className="split-qty">{entry.units}</div>
                        <div className="split-units">units · ₹{entry.total_cost?.toFixed(0)} · {entry.delivery_time}h</div>
                      </div>
                    </div>
                  ))}

                  <div className="result-panel" style={{ marginTop: '12px' }}>
                    <div className="result-header">📊 Split Summary</div>
                    <div className="result-row">
                      <span className="label">Total Cost</span>
                      <span className="value" style={{ color: 'var(--accent-emerald)' }}>
                        ₹{result.total_cost?.toFixed(0)}
                      </span>
                    </div>
                    <div className="result-row">
                      <span className="label">Max Delivery Time</span>
                      <span className="value">{result.max_delivery_time}h</span>
                    </div>
                    <div className="result-row">
                      <span className="label">Warehouses Used</span>
                      <span className="value">{result.plan?.length}</span>
                    </div>
                  </div>
                </>
              ) : (
                <div className="result-panel" style={{ background: 'rgba(244, 63, 94, 0.05)', borderColor: 'rgba(244, 63, 94, 0.2)' }}>
                  <div className="result-header" style={{ color: 'var(--accent-rose)' }}>
                    ❌ Cannot Fulfill Order
                  </div>
                  <p style={{ fontSize: '13px', color: 'var(--text-secondary)' }}>
                    Insufficient total stock across all warehouses for this quantity.
                  </p>
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
