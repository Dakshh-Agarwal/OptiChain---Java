# OptiFlow 🚚 📦 
### Distributed Inventory Optimization & Delivery Engine

[![C++](https://img.shields.io/badge/C++-Advanced_DSA-00599C?logo=c%2B%2B)](https://cplusplus.com/)
[![React](https://img.shields.io/badge/React-Glassmorphism_UI-61DAFB?logo=react)](https://react.dev)
[![Flask](https://img.shields.io/badge/Python-Flask_Bridge-000000?logo=flask)](https://flask.palletsprojects.com/)
[![Algorithms](https://img.shields.io/badge/Data_Structures-Dijkstra_|_Fenwick_|_DP-8A2BE2)](#-core-algorithmic-modules)

> **OptiFlow** is a highly-optimized, interview-defensible supply chain simulation engine. It intelligently routes deliveries, manages real-time warehouse inventory, processes split-orders dynamically, and predicts demand trends using advanced Data Structures and Algorithms (DSA) operating seamlessly through a high-performance C++ backend.

---

## 🔥 System Architecture 

OptiFlow is separated into a tightly coupled trinity of performance layers designed to simulate enterprise-grade routing complexity at ultra-low latencies:

1. **C++ DSA Core Processing (`/engine`)** — The algorithmic brain of the engine. Bypasses external databases to process massive `stdin`/`stdout` JSON graphs purely in-memory using heavily optimized complex heuristics.
2. **Python Bridge API (`/backend`)** — A lightweight HTTP translation server wrapping the C++ subprocess in a RESTful Flask network. 
3. **React Visualizer (`/frontend`)** — A strictly modern, responsive glassmorphism dark-theme interface dynamically bridging the logistics visualizations (leveraging HTML5 Canvas mapping algorithms) into a stunning client presentation.

<br/>

## 🧠 Core Algorithmic Modules

### 1. 🔀 Shortest Path & Logistics Routing (`O((V+E) log V)`)
* **Algorithm:** Dijkstra’s Algorithm & Breadth-First Search (BFS)
* **Design:** Calculates the absolute lowest-cost freight routes acting on Adjacency Lists populated by weighted geospatial edges traversing national warehouse networks. BFS sits as a fallback calculating optimal unit-hop pathways.

### 2. ⚡ Real-Time Inventory Control (`O(log N)`)
* **Data Structure:** Fenwick Tree (Binary Indexed Tree)
* **Design:** Allows sub-millisecond precision over massive `point updates` representing dynamic inventory consumption, and `range sum query` operations extracting active logistical payloads across vast country-wide zones.

### 3. 🎯 Priority Order Allocation 
* **Data Structure:** Min-Heap (Priority Queue) 
* **Design:** Aggregates delivery-candidate data simultaneously ranking warehouses against transit costs, transit times, and immediate stock depth to automatically assign standard orders to the most absolute optimal delivery origin.

### 4. 🧩 Partition DP: Split Order Engine (`O(U × W × K)`)
* **Algorithm:** Partition Dynamic Programming (Bounded Knapsack Variant)
* **Design:** Kicks in gracefully when *no single warehouse* holds enough stock to fulfill an enterprise order. It utilizes constrained backtracking algorithms dynamically calculating the cheapest split combination of mixed-origin units mathematically possible.

### 5. 📈 Predictive Demand Analysis 
* **Data Structure:** Sliding Window Deques & HashMaps
* **Design:** Actively intercepts localized stock drains computing a Weighted Moving Average. Weights are recursively targeted toward recent consumption velocity alerting managers immediately against statistical volatility thresholds.

---

## 🚀 Installation & Setup

Ensure your local environment possesses typical dependencies installed: **C++ Compiler (MinGW/GCC/MSVC)**, **Python 3.10+**, **Node.js 18+**, and **CMake**.

### 1. Compile the C++ Core
```bash
cd engine
# Assuming MinGW/GCC is installed:
g++ -O2 -std=c++11 .\main.cpp .\graph.cpp .\fenwick.cpp .\allocator.cpp .\splitter.cpp .\demand.cpp -o .\build\engine.exe
```

### 2. Launch the Python Bridge
```bash
cd backend
python -m pip install -r requirements.txt
python app.py
```
*(Runs seamlessly on `http://localhost:5000`)*

### 3. Launch the Visual Frontend
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
