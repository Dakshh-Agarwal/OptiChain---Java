"""
Python Flask Bridge — connects React frontend to C++ DSA Engine.

This is a thin bridge layer. All heavy computation happens in the C++ binary.
Python simply:
  1. Receives HTTP requests from the frontend
  2. Sends JSON commands to the C++ engine via stdin
  3. Reads JSON responses from stdout
  4. Returns them to the frontend

The C++ engine runs as a long-lived subprocess.
"""

import subprocess
import json
import os
import sys
import threading
from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

# ──────────────────────────────────────────────
# C++ Engine Process Manager
# ──────────────────────────────────────────────
class EngineProcess:
    def __init__(self):
        self.process = None
        self.lock = threading.Lock()
    
    def start(self, engine_path, data_dir):
        """Start the C++ engine subprocess."""
        self.process = subprocess.Popen(
            [engine_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1  # line buffered
        )
        # Initialize engine with data directory
        result = self.send_command({"cmd": "init", "dataDir": data_dir})
        print(f"[Engine] Initialized: {result}")
        return result
    
    def send_command(self, cmd):
        """Send a JSON command to the engine and get response."""
        with self.lock:
            try:
                cmd_str = json.dumps(cmd) + "\n"
                self.process.stdin.write(cmd_str)
                self.process.stdin.flush()
                
                response_line = self.process.stdout.readline().strip()
                if not response_line:
                    return {"error": "Empty response from engine"}
                
                return json.loads(response_line)
            except Exception as e:
                return {"error": str(e)}
    
    def stop(self):
        if self.process:
            self.send_command({"cmd": "quit"})
            self.process.terminate()

engine = EngineProcess()

# ──────────────────────────────────────────────
# API Routes
# ──────────────────────────────────────────────

@app.route("/api/health", methods=["GET"])
def health():
    return jsonify({"status": "ok", "engine": engine.process is not None})

@app.route("/api/graph", methods=["GET"])
def get_graph():
    """Get full graph data (nodes + edges) for visualization."""
    return jsonify(engine.send_command({"cmd": "graph_data"}))

@app.route("/api/warehouses", methods=["GET"])
def get_warehouses():
    """Get all warehouse nodes."""
    return jsonify(engine.send_command({"cmd": "warehouses"}))

@app.route("/api/dijkstra", methods=["POST"])
def dijkstra():
    """Find shortest path between two warehouses using Dijkstra."""
    data = request.json
    return jsonify(engine.send_command({
        "cmd": "dijkstra",
        "from": data["from"],
        "to": data["to"]
    }))

@app.route("/api/bfs", methods=["POST"])
def bfs():
    """Find shortest hop path between two warehouses using BFS."""
    data = request.json
    return jsonify(engine.send_command({
        "cmd": "bfs",
        "from": data["from"],
        "to": data["to"]
    }))

@app.route("/api/inventory", methods=["GET"])
def get_inventory():
    """Get stock for a specific product across all warehouses."""
    product = request.args.get("product", "P001")
    return jsonify(engine.send_command({
        "cmd": "inventory",
        "product": product
    }))

@app.route("/api/inventory/all", methods=["GET"])
def get_all_inventory():
    """Get full inventory snapshot."""
    return jsonify(engine.send_command({"cmd": "all_inventory"}))

@app.route("/api/allocate", methods=["POST"])
def allocate_order():
    """Find best warehouse(s) to fulfill an order."""
    data = request.json
    return jsonify(engine.send_command({
        "cmd": "allocate",
        "product": data["product"],
        "qty": data["quantity"],
        "dest": data["destination"]
    }))

@app.route("/api/split", methods=["POST"])
def split_order():
    """Split an order across multiple warehouses using DP."""
    data = request.json
    return jsonify(engine.send_command({
        "cmd": "split",
        "product": data["product"],
        "qty": data["quantity"],
        "dest": data["destination"]
    }))

@app.route("/api/demand/record", methods=["POST"])
def record_demand():
    """Record demand for a product."""
    data = request.json
    return jsonify(engine.send_command({
        "cmd": "demand_record",
        "product": data["product"],
        "qty": data["quantity"]
    }))

@app.route("/api/demand/trend", methods=["GET"])
def get_demand_trend():
    """Get demand trend for a product."""
    product = request.args.get("product", "P001")
    return jsonify(engine.send_command({
        "cmd": "demand_trend",
        "product": product
    }))

@app.route("/api/demand/alerts", methods=["GET"])
def get_demand_alerts():
    """Get all restock alerts."""
    return jsonify(engine.send_command({"cmd": "demand_alerts"}))

# ──────────────────────────────────────────────
# Startup
# ──────────────────────────────────────────────
if __name__ == "__main__":
    # Determine paths
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    data_dir = os.path.join(project_root, "data")
    
    # Try to find engine binary
    engine_path = os.path.join(project_root, "engine", "build", "Release", "engine.exe")
    if not os.path.exists(engine_path):
        engine_path = os.path.join(project_root, "engine", "build", "engine.exe")
    if not os.path.exists(engine_path):
        engine_path = os.path.join(project_root, "engine", "build", "Debug", "engine.exe")
    if not os.path.exists(engine_path):
        print(f"[ERROR] Engine binary not found. Build it first!")
        print(f"  cd engine && mkdir build && cd build && cmake .. && cmake --build . --config Release")
        sys.exit(1)
    
    print(f"[Bridge] Engine: {engine_path}")
    print(f"[Bridge] Data: {data_dir}")
    
    # Start engine
    engine.start(engine_path, data_dir.replace("\\", "/"))
    
    # Run Flask
    print("[Bridge] Starting Flask on http://localhost:5000")
    app.run(host="0.0.0.0", port=5000, debug=False)
