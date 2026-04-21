import { useState } from 'react'
import GraphView from './pages/GraphView'
import OrderPanel from './pages/OrderPanel'
import InventoryView from './pages/InventoryView'
import DemandTracker from './pages/DemandTracker'
import Dashboard from './pages/Dashboard'

const NAV_ITEMS = [
  { id: 'dashboard', label: 'Dashboard', icon: '⬡', section: 'Overview' },
  { id: 'graph', label: 'Network Graph', icon: '◈', section: 'DSA Modules' },
  { id: 'order', label: 'Order Engine', icon: '⚡', section: 'DSA Modules' },
  { id: 'inventory', label: 'Inventory', icon: '▦', section: 'DSA Modules' },
  { id: 'demand', label: 'Demand Tracker', icon: '◉', section: 'DSA Modules' },
];

const PAGES = {
  dashboard: Dashboard,
  graph: GraphView,
  order: OrderPanel,
  inventory: InventoryView,
  demand: DemandTracker,
};

const PAGE_INFO = {
  dashboard: { title: 'System Overview', subtitle: 'Real-time metrics across all modules' },
  graph: { title: 'Logistics Network', subtitle: 'Dijkstra\'s Algorithm  ·  O((V+E) log V)' },
  order: { title: 'Order Fulfillment', subtitle: 'Min-Heap Allocation  ·  Partition DP Split' },
  inventory: { title: 'Inventory Manager', subtitle: 'Fenwick Tree  ·  O(log n) Range Queries' },
  demand: { title: 'Demand Analytics', subtitle: 'Sliding Window Deque  ·  Weighted Moving Average' },
};

export default function App() {
  const [activePage, setActivePage] = useState('dashboard');
  const PageComponent = PAGES[activePage];
  const pageInfo = PAGE_INFO[activePage];

  let currentSection = '';

  return (
    <div className="app-layout">
      {/* Sidebar */}
      <aside className="sidebar">
        <div className="sidebar-logo">
          <div className="logo-icon">O</div>
          <div>
            <div className="logo-text">OptiFlow</div>
            <div className="logo-sub">DSA Engine v1.0</div>
          </div>
        </div>

        <nav className="sidebar-nav">
          {NAV_ITEMS.map((item) => {
            const showSection = item.section !== currentSection;
            if (showSection) currentSection = item.section;
            return (
              <div key={item.id}>
                {showSection && (
                  <div className="nav-section-title">{item.section}</div>
                )}
                <div
                  className={`nav-item ${activePage === item.id ? 'active' : ''}`}
                  onClick={() => setActivePage(item.id)}
                >
                  <span className="nav-icon">{item.icon}</span>
                  {item.label}
                </div>
              </div>
            );
          })}
        </nav>

        <div className="sidebar-footer">
          <div className="dsa-badge">
            <strong>Core DSA:</strong> Dijkstra · BFS · Fenwick Tree · Min-Heap · Partition DP · Sliding Window
          </div>
        </div>
      </aside>

      {/* Main Content */}
      <main className="main-content">
        <header className="content-header">
          <div>
            <h1>{pageInfo.title}</h1>
            <span className="header-subtitle">{pageInfo.subtitle}</span>
          </div>
          <div style={{ display: 'flex', alignItems: 'center', gap: '12px' }}>
            <span className="badge badge-success">● Engine Online</span>
          </div>
        </header>

        <div className="content-body">
          <PageComponent />
        </div>
      </main>
    </div>
  );
}
