import { useState, useEffect } from 'react';
import { api } from '../api';

export default function InventoryView() {
  const [products, setProducts] = useState([]);
  const [selectedProduct, setSelectedProduct] = useState(null);
  const [productStock, setProductStock] = useState(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    loadInventory();
  }, []);

  async function loadInventory() {
    try {
      const data = await api.getAllInventory();
      const sorted = (data.products || []).sort((a, b) => a.id.localeCompare(b.id));
      setProducts(sorted);
      if (sorted.length > 0) {
        selectProduct(sorted[0].id);
      }
    } catch (err) {
      console.error('Failed to load inventory:', err);
    } finally {
      setLoading(false);
    }
  }

  async function selectProduct(pid) {
    setSelectedProduct(pid);
    try {
      const data = await api.getInventory(pid);
      setProductStock(data);
    } catch (err) {
      console.error('Failed to load product stock:', err);
    }
  }

  if (loading) {
    return (
      <div className="loading-overlay">
        <div className="spinner"></div>
        Loading inventory...
      </div>
    );
  }

  const maxStock = Math.max(...(productStock?.warehouses || []).map(w => w.stock), 1);

  return (
    <div className="fade-in">
      <div className="algo-tags">
        <span className="algo-tag">Fenwick Tree (BIT)</span>
        <span className="algo-tag">Point Update O(log n)</span>
        <span className="algo-tag">Range Query O(log n)</span>
        <span className="algo-tag">Prefix Sum</span>
      </div>

      <div className="grid-2-1">
        {/* Product Table */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">📦</span>
            Product Catalog
          </div>
          
          <div style={{ maxHeight: '500px', overflowY: 'auto' }}>
            <table className="data-table">
              <thead>
                <tr>
                  <th>ID</th>
                  <th>Product</th>
                  <th>Price</th>
                  <th>Total Stock</th>
                  <th>Status</th>
                </tr>
              </thead>
              <tbody>
                {products.map(p => {
                  const isSelected = selectedProduct === p.id;
                  return (
                    <tr 
                      key={p.id} 
                      onClick={() => selectProduct(p.id)}
                      style={{ 
                        cursor: 'pointer',
                        background: isSelected ? 'rgba(99, 102, 241, 0.08)' : 'transparent',
                      }}
                    >
                      <td style={{ fontWeight: 600, color: 'var(--accent-indigo)' }}>{p.id}</td>
                      <td>{p.name}</td>
                      <td>₹{p.unit_price?.toLocaleString()}</td>
                      <td style={{ fontWeight: 600 }}>{p.total_stock?.toLocaleString()}</td>
                      <td>
                        {p.total_stock > 500 
                          ? <span className="badge badge-success">In Stock</span>
                          : p.total_stock > 200 
                            ? <span className="badge badge-warning">Low</span>
                            : <span className="badge badge-danger">Critical</span>
                        }
                      </td>
                    </tr>
                  );
                })}
              </tbody>
            </table>
          </div>
        </div>

        {/* Warehouse Stock Detail */}
        <div className="card">
          <div className="card-title">
            <span className="card-icon">🏭</span>
            Warehouse Stock
            {selectedProduct && (
              <span className="badge badge-info" style={{ marginLeft: 'auto' }}>{selectedProduct}</span>
            )}
          </div>

          {productStock && (
            <>
              <div style={{ 
                padding: '12px', 
                background: 'rgba(0,0,0,0.2)', 
                borderRadius: '8px', 
                marginBottom: '16px',
                textAlign: 'center'
              }}>
                <div style={{ fontSize: '28px', fontWeight: 700 }}>
                  {productStock.total_stock?.toLocaleString()}
                </div>
                <div style={{ fontSize: '12px', color: 'var(--text-muted)' }}>
                  Total units across all warehouses
                </div>
                <div className="complexity-box" style={{ marginTop: '8px', fontSize: '11px' }}>
                  Fenwick query(1, 25) = {productStock.total_stock} → O(log 25)
                </div>
              </div>

              <div style={{ maxHeight: '350px', overflowY: 'auto' }}>
                {(productStock.warehouses || []).map(w => {
                  const pct = (w.stock / maxStock) * 100;
                  const level = pct > 60 ? 'high' : pct > 30 ? 'medium' : 'low';
                  return (
                    <div key={w.warehouse_id} style={{ 
                      padding: '8px 0', 
                      borderBottom: '1px solid rgba(255,255,255,0.03)' 
                    }}>
                      <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '4px' }}>
                        <span style={{ fontSize: '12px', color: 'var(--text-secondary)' }}>
                          {w.name || `WH ${w.warehouse_id}`}
                          <span style={{ color: 'var(--text-muted)', marginLeft: '6px' }}>{w.city}</span>
                        </span>
                        <span style={{ fontSize: '12px', fontWeight: 600 }}>{w.stock}</span>
                      </div>
                      <div className="stock-bar-container">
                        <div className="stock-bar">
                          <div className={`stock-bar-fill ${level}`} style={{ width: `${pct}%` }} />
                        </div>
                      </div>
                    </div>
                  );
                })}
              </div>
            </>
          )}
        </div>
      </div>
    </div>
  );
}
