import { useState, useEffect, useRef } from 'react';
import { api } from '../api';

export default function GraphView() {
  const canvasRef = useRef(null);
  const [graph, setGraph] = useState(null);
  const [warehouses, setWarehouses] = useState({});
  const [fromId, setFromId] = useState(1);
  const [toId, setToId] = useState(5);
  const [pathResult, setPathResult] = useState(null);
  const [algorithm, setAlgorithm] = useState('dijkstra');
  const [loading, setLoading] = useState(false);
  const [highlightedPath, setHighlightedPath] = useState([]);
  const [nodePositions, setNodePositions] = useState({});

  useEffect(() => {
    loadGraph();
  }, []);

  useEffect(() => {
    if (graph && canvasRef.current) {
      drawGraph();
    }
  }, [graph, highlightedPath]);

  async function loadGraph() {
    try {
      const data = await api.getGraph();
      setGraph(data);
      
      // Build warehouse lookup
      const whMap = {};
      data.nodes.forEach(n => { whMap[n.id] = n; });
      setWarehouses(whMap);
    } catch (err) {
      console.error('Failed to load graph:', err);
    }
  }

  function drawGraph() {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    const rect = canvas.parentElement.getBoundingClientRect();
    canvas.width = rect.width;
    canvas.height = rect.height;

    const W = canvas.width;
    const H = canvas.height;
    const padding = 50;

    // Compute positions from lat/lng
    const nodes = graph.nodes || [];
    if (nodes.length === 0) return;

    let minLat = Infinity, maxLat = -Infinity;
    let minLng = Infinity, maxLng = -Infinity;
    nodes.forEach(n => {
      minLat = Math.min(minLat, n.lat);
      maxLat = Math.max(maxLat, n.lat);
      minLng = Math.min(minLng, n.lng);
      maxLng = Math.max(maxLng, n.lng);
    });

    const positions = {};
    nodes.forEach(n => {
      const x = padding + ((n.lng - minLng) / (maxLng - minLng || 1)) * (W - 2 * padding);
      const y = padding + ((maxLat - n.lat) / (maxLat - minLat || 1)) * (H - 2 * padding);
      positions[n.id] = { x, y };
    });
    setNodePositions(positions);

    // Clear
    ctx.fillStyle = '#080c16';
    ctx.fillRect(0, 0, W, H);

    // Draw grid dots
    ctx.fillStyle = 'rgba(255,255,255,0.03)';
    for (let gx = 0; gx < W; gx += 30) {
      for (let gy = 0; gy < H; gy += 30) {
        ctx.beginPath();
        ctx.arc(gx, gy, 1, 0, Math.PI * 2);
        ctx.fill();
      }
    }

    // Highlighted path edges set
    const pathEdgeSet = new Set();
    for (let i = 0; i + 1 < highlightedPath.length; i++) {
      const a = Math.min(highlightedPath[i], highlightedPath[i + 1]);
      const b = Math.max(highlightedPath[i], highlightedPath[i + 1]);
      pathEdgeSet.add(`${a}-${b}`);
    }

    // Draw edges
    const edges = graph.edges || [];
    edges.forEach(e => {
      const p1 = positions[e.from];
      const p2 = positions[e.to];
      if (!p1 || !p2) return;

      const a = Math.min(e.from, e.to);
      const b = Math.max(e.from, e.to);
      const isHighlighted = pathEdgeSet.has(`${a}-${b}`);

      ctx.beginPath();
      ctx.moveTo(p1.x, p1.y);
      ctx.lineTo(p2.x, p2.y);

      if (isHighlighted) {
        ctx.strokeStyle = '#6366f1';
        ctx.lineWidth = 3;
        ctx.shadowColor = '#6366f1';
        ctx.shadowBlur = 12;
      } else {
        ctx.strokeStyle = 'rgba(255,255,255,0.08)';
        ctx.lineWidth = 1;
        ctx.shadowBlur = 0;
      }
      ctx.stroke();
      ctx.shadowBlur = 0;
    });

    // Draw nodes
    const pathNodeSet = new Set(highlightedPath);
    nodes.forEach(n => {
      const pos = positions[n.id];
      if (!pos) return;

      const isOnPath = pathNodeSet.has(n.id);
      const isSource = highlightedPath.length > 0 && highlightedPath[0] === n.id;
      const isDest = highlightedPath.length > 0 && highlightedPath[highlightedPath.length - 1] === n.id;

      // Glow
      if (isOnPath) {
        ctx.beginPath();
        ctx.arc(pos.x, pos.y, 16, 0, Math.PI * 2);
        const glow = ctx.createRadialGradient(pos.x, pos.y, 4, pos.x, pos.y, 16);
        glow.addColorStop(0, 'rgba(99,102,241,0.4)');
        glow.addColorStop(1, 'rgba(99,102,241,0)');
        ctx.fillStyle = glow;
        ctx.fill();
      }

      // Node circle
      ctx.beginPath();
      ctx.arc(pos.x, pos.y, isOnPath ? 8 : 5, 0, Math.PI * 2);
      
      if (isSource) {
        ctx.fillStyle = '#10b981';
      } else if (isDest) {
        ctx.fillStyle = '#f43f5e';
      } else if (isOnPath) {
        ctx.fillStyle = '#6366f1';
      } else {
        ctx.fillStyle = '#374151';
      }
      ctx.fill();

      // Border
      ctx.strokeStyle = isOnPath ? 'rgba(255,255,255,0.5)' : 'rgba(255,255,255,0.15)';
      ctx.lineWidth = isOnPath ? 2 : 1;
      ctx.stroke();

      // Label
      ctx.fillStyle = isOnPath ? '#f1f5f9' : 'rgba(255,255,255,0.4)';
      ctx.font = isOnPath ? '600 11px Inter' : '400 10px Inter';
      ctx.textAlign = 'center';
      ctx.fillText(n.city, pos.x, pos.y - (isOnPath ? 14 : 10));
    });
  }

  async function findPath() {
    setLoading(true);
    try {
      let result;
      if (algorithm === 'dijkstra') {
        result = await api.dijkstra(fromId, toId);
      } else {
        result = await api.bfs(fromId, toId);
      }
      setPathResult(result);
      setHighlightedPath(result.path || []);
    } catch (err) {
      console.error('Path finding failed:', err);
    } finally {
      setLoading(false);
    }
  }

  const warehouseList = Object.values(warehouses).sort((a, b) => a.id - b.id);

  return (
    <div className="fade-in">
      <div className="algo-tags">
        <span className="algo-tag">Adjacency List</span>
        <span className="algo-tag">Dijkstra O((V+E)logV)</span>
        <span className="algo-tag">BFS O(V+E)</span>
        <span className="algo-tag">Min Heap</span>
      </div>

      <div className="grid-2-1">
        {/* Graph Canvas */}
        <div>
          <div className="graph-container">
            <canvas ref={canvasRef} />
          </div>

          {/* Path Timeline */}
          {pathResult?.reachable && pathResult.path_details && (
            <div className="result-panel" style={{ marginTop: '16px' }}>
              <div className="result-header">📍 Route Path</div>
              <div className="path-timeline">
                {pathResult.path_details.map((node, i) => (
                  <div key={i} style={{ display: 'flex', alignItems: 'center' }}>
                    <div className="path-node">
                      <div className="node-dot" style={{
                        background: i === 0 ? 'linear-gradient(135deg, #10b981, #06b6d4)' 
                          : i === pathResult.path_details.length - 1 ? 'linear-gradient(135deg, #f43f5e, #e11d48)'
                          : 'var(--gradient-primary)'
                      }}>
                        {node.id}
                      </div>
                      <span className="node-label">{node.city || `WH ${node.id}`}</span>
                    </div>
                    {i < pathResult.path_details.length - 1 && (
                      <div className="path-connector" />
                    )}
                  </div>
                ))}
              </div>
            </div>
          )}
        </div>

        {/* Controls */}
        <div>
          <div className="card" style={{ marginBottom: '16px' }}>
            <div className="card-title">
              <span className="card-icon">🔍</span>
              Path Finder
            </div>

            <div className="form-group">
              <label className="form-label">Algorithm</label>
              <select className="form-select" value={algorithm} onChange={e => setAlgorithm(e.target.value)}>
                <option value="dijkstra">Dijkstra (Weighted)</option>
                <option value="bfs">BFS (Hop Count)</option>
              </select>
            </div>

            <div className="form-group">
              <label className="form-label">Source Warehouse</label>
              <select className="form-select" value={fromId} onChange={e => setFromId(Number(e.target.value))}>
                {warehouseList.map(w => (
                  <option key={w.id} value={w.id}>{w.id} — {w.city}</option>
                ))}
              </select>
            </div>

            <div className="form-group">
              <label className="form-label">Destination</label>
              <select className="form-select" value={toId} onChange={e => setToId(Number(e.target.value))}>
                {warehouseList.map(w => (
                  <option key={w.id} value={w.id}>{w.id} — {w.city}</option>
                ))}
              </select>
            </div>

            <button className="btn btn-primary btn-block" onClick={findPath} disabled={loading}>
              {loading ? <><div className="spinner"></div> Computing...</> : '⚡ Find Optimal Route'}
            </button>
          </div>

          {/* Result */}
          {pathResult && (
            <div className="card">
              <div className="card-title">
                <span className="card-icon">📊</span>
                Result
              </div>

              {pathResult.reachable ? (
                <>
                  <div className="result-row">
                    <span className="label">Status</span>
                    <span className="badge badge-success">Reachable</span>
                  </div>
                  <div className="result-row">
                    <span className="label">Total Cost</span>
                    <span className="value" style={{ color: 'var(--accent-emerald)' }}>
                      ₹{pathResult.total_cost?.toLocaleString()}
                    </span>
                  </div>
                  <div className="result-row">
                    <span className="label">Transit Time</span>
                    <span className="value">{pathResult.total_time}h</span>
                  </div>
                  <div className="result-row">
                    <span className="label">Hops</span>
                    <span className="value">{(pathResult.path?.length || 1) - 1}</span>
                  </div>
                  {pathResult.hops !== undefined && (
                    <div className="result-row">
                      <span className="label">BFS Hops</span>
                      <span className="value">{pathResult.hops}</span>
                    </div>
                  )}
                </>
              ) : (
                <div className="result-row">
                  <span className="label">Status</span>
                  <span className="badge badge-danger">Unreachable</span>
                </div>
              )}
            </div>
          )}
        </div>
      </div>
    </div>
  );
}
