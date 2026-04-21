const API_BASE = 'http://localhost:5000/api';

async function request(path, options = {}) {
  const url = `${API_BASE}${path}`;
  const config = {
    headers: { 'Content-Type': 'application/json' },
    ...options,
  };
  
  const res = await fetch(url, config);
  if (!res.ok) throw new Error(`API Error: ${res.status}`);
  return res.json();
}

export const api = {
  // Health check
  health: () => request('/health'),
  
  // Graph data
  getGraph: () => request('/graph'),
  getWarehouses: () => request('/warehouses'),
  
  // Routing
  dijkstra: (from, to) => request('/dijkstra', {
    method: 'POST',
    body: JSON.stringify({ from, to }),
  }),
  bfs: (from, to) => request('/bfs', {
    method: 'POST',
    body: JSON.stringify({ from, to }),
  }),
  
  // Inventory
  getInventory: (product) => request(`/inventory?product=${product}`),
  getAllInventory: () => request('/inventory/all'),
  
  // Order allocation
  allocate: (product, quantity, destination) => request('/allocate', {
    method: 'POST',
    body: JSON.stringify({ product, quantity, destination }),
  }),
  
  // Order splitting (DP)
  split: (product, quantity, destination) => request('/split', {
    method: 'POST',
    body: JSON.stringify({ product, quantity, destination }),
  }),
  
  // Demand tracking
  recordDemand: (product, quantity) => request('/demand/record', {
    method: 'POST',
    body: JSON.stringify({ product, quantity }),
  }),
  getDemandTrend: (product) => request(`/demand/trend?product=${product}`),
  getDemandAlerts: () => request('/demand/alerts'),
};
