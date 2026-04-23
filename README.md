# OptiFlow 🚚 📦 
### Distributed Inventory Optimization & Delivery Engine

[![Java](https://img.shields.io/badge/Java-Advanced_DSA-ED8B00?logo=openjdk)](https://openjdk.org/)
[![React](https://img.shields.io/badge/React-Glassmorphism_UI-61DAFB?logo=react)](https://react.dev)
[![Algorithms](https://img.shields.io/badge/Data_Structures-Dijkstra_|_Fenwick_|_DP-8A2BE2)](#-core-algorithmic-modules)

> **OptiFlow** is a highly-optimized, interview-defensible supply chain simulation engine. It intelligently routes deliveries, manages real-time warehouse inventory, processes split-orders dynamically, and predicts demand trends using advanced Data Structures and Algorithms (DSA) operating seamlessly through a high-performance **Java** backend.

---

## 🔥 System Architecture 

OptiFlow is separated into two tightly coupled layers:

1. **Java DSA Engine + HTTP Server (`/engine`)** — The algorithmic brain. Implements all DSA modules (Dijkstra, Fenwick Tree, Min-Heap, Partition DP, Sliding Window) and serves a REST API using Java's built-in `HttpServer` — zero external dependencies.
2. **React Visualizer (`/frontend`)** — A modern, responsive glassmorphism dark-theme interface dynamically bridging the logistics visualizations (leveraging HTML5 Canvas) into a stunning client presentation.

<br/>

## 🧠 Core Algorithmic Modules

### 1. 🔀 Shortest Path & Logistics Routing (`O((V+E) log V)`)
* **Algorithm:** Dijkstra's Algorithm & Breadth-First Search (BFS)
* **Data Structure:** Adjacency List, PriorityQueue (Min-Heap)
* **Design:** Calculates the absolute lowest-cost freight routes acting on Adjacency Lists populated by weighted geospatial edges traversing national warehouse networks. BFS sits as a fallback calculating optimal unit-hop pathways.

### 2. ⚡ Real-Time Inventory Control (`O(log N)`)
* **Data Structure:** Fenwick Tree (Binary Indexed Tree)
* **Design:** Allows sub-millisecond precision over massive `point updates` representing dynamic inventory consumption, and `range sum query` operations extracting active logistical payloads across vast country-wide zones.

### 3. 🎯 Priority Order Allocation 
* **Data Structure:** Min-Heap (PriorityQueue) 
* **Design:** Aggregates delivery-candidate data simultaneously ranking warehouses against transit costs, transit times, and immediate stock depth to automatically assign standard orders to the most absolute optimal delivery origin.

### 4. 🧩 Partition DP: Split Order Engine (`O(U × W × K)`)
* **Algorithm:** Partition Dynamic Programming (Bounded Knapsack Variant)
* **Design:** Kicks in gracefully when *no single warehouse* holds enough stock to fulfill an enterprise order. It utilizes constrained backtracking algorithms dynamically calculating the cheapest split combination of mixed-origin units mathematically possible.

### 5. 📈 Predictive Demand Analysis 
* **Data Structure:** Sliding Window Deques & HashMaps
* **Design:** Actively intercepts localized stock drains computing a Weighted Moving Average. Weights are recursively targeted toward recent consumption velocity alerting managers immediately against statistical volatility thresholds.

---

## 🚀 Installation & Setup

Ensure your local environment has: **Java 11+** (JDK) and **Node.js 18+**.

### 1. Compile & Run the Java Engine
```bash
cd engine
# Compile all Java sources
javac -d build src/main/java/com/optiflow/engine/*.java

# Run the server
java -cp build com.optiflow.engine.OptiFlowServer
```
*(Runs on `http://localhost:5000`)*

### 2. Launch the Visual Frontend
```bash
cd frontend
npm install
npm run dev
```
*(Web Application will be served on `http://localhost:5173`)*

---

## 📸 Overview Interface
*(Attach Screenshots Here)*

* Built completely out of passion for computational bounds. *
