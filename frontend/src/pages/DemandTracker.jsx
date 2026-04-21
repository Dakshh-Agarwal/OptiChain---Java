import { useState, useEffect } from 'react';
import { api } from '../api';

export default function DemandTracker() {
  const [products, setProducts] = useState([]);
  const [selectedProduct, setSelectedProduct] = useState('P001');
  const [quantity, setQuantity] = useState(30);
  const [trend, setTrend] = useState(null);
  const [alerts, setAlerts] = useState([]);
  const [loading, setLoading] = useState(false);
  const [history, setHistory] = useState([]);

  useEffect(() => {
    loadProducts();
    loadAlerts();
  }, []);

  useEffect(() => {
    loadTrend(selectedProduct);
  }, [selectedProduct]);

  async function loadProducts() {
    try {
      const data = await api.getAllInventory();
      setProducts((data.products || []).sort((a, b) => a.id.localeCompare(b.id)));
    } catch (err) {
      console.error('Failed to load products:', err);
    }
  }

  async function loadTrend(pid) {
    try {
      const data = await api.getDemandTrend(pid);
      setTrend(data);
    } catch (err) {
      console.error('Failed to load trend:', err);
    }
  }

  async function loadAlerts() {
    try {
      const data = await api.getDemandAlerts();
      setAlerts(data.alerts || []);
    } catch (err) {
      console.error('Failed to load alerts:', err);
    }
  }

  async function recordDemand() {
    setLoading(true);
    try {
      await api.recordDemand(selectedProduct, quantity);
      setHistory(prev => [...prev, { product: selectedProduct, qty: quantity, time: new Date().toLocaleTimeString() }]);
      
      // Refresh
      await loadTrend(selectedProduct);
      await loadAlerts();
    } catch (err) {
      console.error('Failed to record demand:', err);
    } finally {
      setLoading(false);
    }
  }

  const maxDemand = Math.max(...(trend?.recent_demand || [1]), 1);

  return (
    <div className="fade-in">
      <div className="algo-tags">
        <span className="algo-tag">Sliding Window</span>
        <span className="algo-tag">Deque</span>
        <span className="algo-tag">Weighted Moving Average</span>
        <span className="algo-tag">HashMap</span>
      </div>

      <div className="grid-2" style={{ marginBottom: '20px' }}>
        {/* Record Demand */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">📝</span>
            Record Demand
          </div>

          <div className="form-group">
            <label className="form-label">Product</label>
            <select className="form-select" value={selectedProduct} onChange={e => setSelectedProduct(e.target.value)}>
              {products.map(p => (
                <option key={p.id} value={p.id}>{p.id} — {p.name}</option>
              ))}
            </select>
          </div>

          <div className="form-group">
            <label className="form-label">Daily Demand (units)</label>
            <input 
              type="number" 
              className="form-input" 
              value={quantity} 
              onChange={e => setQuantity(Number(e.target.value))}
              min={1}
            />
          </div>

          <button className="btn btn-primary btn-block" onClick={recordDemand} disabled={loading}>
            {loading ? <><div className="spinner"></div> Recording...</> : '📊 Record & Analyze'}
          </button>

          {/* Formula */}
          <div className="complexity-box" style={{ marginTop: '16px', fontSize: '11px', lineHeight: 1.6 }}>
            <div>weight[i] = (i+1) / Σ(1..N)</div>
            <div>trend = Σ(demand[i] × weight[i])</div>
            <div style={{ color: 'var(--text-muted)', marginTop: '4px' }}>Recent days weighted more heavily</div>
          </div>
        </div>

        {/* Trend Result */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">📈</span>
            Trend Analysis — {selectedProduct}
          </div>

          {trend && trend.recent_demand?.length > 0 ? (
            <>
              <div className="stats-grid" style={{ gridTemplateColumns: '1fr 1fr', marginBottom: '16px' }}>
                <div style={{ padding: '12px', background: 'rgba(0,0,0,0.2)', borderRadius: '8px', textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 700, color: 'var(--accent-indigo)' }}>
                    {trend.weighted_average?.toFixed(1)}
                  </div>
                  <div style={{ fontSize: '11px', color: 'var(--text-muted)' }}>Weighted Avg</div>
                </div>
                <div style={{ padding: '12px', background: 'rgba(0,0,0,0.2)', borderRadius: '8px', textAlign: 'center' }}>
                  <div style={{ fontSize: '24px', fontWeight: 700 }}>
                    {trend.total_demand?.toFixed(0)}
                  </div>
                  <div style={{ fontSize: '11px', color: 'var(--text-muted)' }}>Total (Window)</div>
                </div>
              </div>

              <div style={{ marginBottom: '12px' }}>
                {trend.needs_restock 
                  ? <span className="badge badge-danger">⚠ Restock Needed</span>
                  : <span className="badge badge-success">✓ Demand Normal</span>
                }
                <span className="badge badge-info" style={{ marginLeft: '6px' }}>
                  Window: {trend.window_size} days
                </span>
              </div>

              {/* Bar Chart */}
              <div style={{ fontSize: '11px', color: 'var(--text-muted)', marginBottom: '8px' }}>
                Recent Demand (Sliding Window Deque)
              </div>
              <div className="bar-chart">
                {trend.recent_demand.map((val, i) => (
                  <div key={i} className="bar-item">
                    <div className="bar-value">{val}</div>
                    <div 
                      className="bar" 
                      style={{ 
                        height: `${Math.max((val / maxDemand) * 140, 4)}px`,
                        opacity: 0.5 + (i / trend.recent_demand.length) * 0.5,
                      }} 
                    />
                    <div className="bar-label">D{i + 1}</div>
                  </div>
                ))}
              </div>
            </>
          ) : (
            <div className="empty-state">
              <div className="empty-icon">📊</div>
              <h3>No Demand Data Yet</h3>
              <p>Record some demand entries to see trends</p>
            </div>
          )}
        </div>
      </div>

      {/* Bottom row */}
      <div className="grid-2">
        {/* Restock Alerts */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">🚨</span>
            Restock Alerts
            {alerts.length > 0 && (
              <span className="badge badge-danger" style={{ marginLeft: 'auto' }}>{alerts.length}</span>
            )}
          </div>

          {alerts.length === 0 ? (
            <div style={{ padding: '24px', textAlign: 'center', color: 'var(--text-muted)', fontSize: '13px' }}>
              No alerts — all products within normal demand range
            </div>
          ) : (
            alerts.map((a, i) => (
              <div key={i} className="split-card">
                <div className="split-info">
                  <div className="split-wh">{a.product}</div>
                  <div className="split-city">Weighted avg: {a.weighted_average?.toFixed(1)}</div>
                </div>
                <span className="badge badge-danger">Restock</span>
              </div>
            ))
          )}
        </div>

        {/* Activity Log */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">📋</span>
            Session Activity Log
          </div>

          {history.length === 0 ? (
            <div style={{ padding: '24px', textAlign: 'center', color: 'var(--text-muted)', fontSize: '13px' }}>
              No activity yet — record demand to see log entries
            </div>
          ) : (
            <div style={{ maxHeight: '300px', overflowY: 'auto' }}>
              {history.slice().reverse().map((item, i) => (
                <div key={i} style={{ 
                  padding: '10px 0', 
                  borderBottom: '1px solid rgba(255,255,255,0.03)',
                  fontSize: '13px',
                  display: 'flex',
                  justifyContent: 'space-between',
                  alignItems: 'center'
                }}>
                  <div>
                    <span style={{ color: 'var(--accent-indigo)', fontWeight: 600 }}>{item.product}</span>
                    <span style={{ color: 'var(--text-muted)' }}> — {item.qty} units</span>
                  </div>
                  <span style={{ fontSize: '11px', color: 'var(--text-muted)' }}>{item.time}</span>
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
