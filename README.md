# Yen's Algorithm over TCP

This project implements a multi-threaded server capable of processing large graphs to find the $K$ shortest loopless paths between two nodes. It features a custom thread pool architecture, low-level socket programming, and hardware-aware optimizations for modern multi-core CPUs.

## Key Features
* **Parallelized Yen's Algorithm:** Computes "spur paths" in parallel using a custom thread pool.
* **Custom TCP Protocol:** Efficient binary serialization for graph transmission.
* **Robust Server Architecture:** Handles `SIGINT` (Ctrl+C) gracefully via `select()`.
* **Hardware Optimized:** Dynamically adjusts thread counts based on server load and `std::thread::hardware_concurrency`.


## The Algorithm

**Yen's Algorithm** computes the $K$ shortest loopless paths in a weighted graph. It works by finding the first shortest path (using Dijkstra), and then iteratively generating "candidate paths" by deviating from the previous best path at every node (the "spur node").

### Complexity
* **Time Complexity:** $O(K \cdot N \cdot (K + (E + N) \log N))$
    * Where $N$ is the vertex count, $E$ is edges.
    * *Note:* This project effectively divides the $N$ term by the number of available threads.
* **Space Complexity:** $O(N + E + K \cdot N)$


## Architecture

### 1. Client-Server over TCP
The project avoids high-level HTTP libraries in favor of raw **BSD Sockets**.
* **Protocol:** Custom binary stream. Data is sent as `[Length][Data]` packets. Integers are network-byte-ordered (`htonl`/`ntohl`).
* **Serialization:** Graphs are serialized as adjacency lists. Path results are streamed back as vector arrays.
* **Concurrency:** The server uses a "Listener Pool" to handle incoming connections. If all listener threads are busy, clients wait in the OS backlog queue.

### 2. Parallelization of Yen
Yen's algorithm is CPU-bound. The most expensive part is calculating new paths from every node in the previous best path.
* **Strategy:** We treat every spur node calculation as an independent task.
* **Implementation:** These tasks are pushed to a `Threadpool`.
* **Optimization:** Instead of spawning 1000s of threads (which causes cache thrashing), we limit the pool size to the physical core count (e.g., 4-16 threads) to maximize L1/L2 cache hits.

### 3. The Threadpool
A custom `Threadpool` class manages worker threads.
* **Server Level:** Manages concurrent clients (e.g., limit to 4 active clients).
* **Algorithm Level:** Manages parallel Dijkstra runs (e.g., 4 worker threads per client).

**Benchmarking results:** The parallelisation of Yen's algorithm yielded improvement in the execution time, most noticably with graph5.txt.

```bash
$ bin/client.out < test/graph5.txt #using 1 thread
(...)
The algorithm took 4111.31ms.

$ bin/client.out < test/graph5.txt #using 4 threads
(...)
The algorithm took 1619.46ms.
```

##  Robustness & Signal Handling
To ensure high availability, the project implements custom signal handling:
* **SIGINT:** The server uses `select()` on the listening socket. This allows the process to catch `Ctrl+C` and shut down gracefully (closing sockets and joining threads) rather than being killed by the OS.
* **SIGPIPE:** Both Server and Client ignore `SIGPIPE`. This prevents the application from crashing if the remote end closes the connection unexpectedly during a data transfer, allowing the code to handle the error via `errno` instead.

## Development Journey

The project was built in iterative stages, tackling specific systems programming challenges at each step:

1.  **Dijkstra Implementation:** Built the core pathfinding engine using priority queues and adjacency lists.
2.  **Yen Implementation:** Extended Dijkstra to find $K$ paths using candidate lists and spur nodes.
3.  **Simple Client-Server:** Established a basic TCP handshake to exchange a single string message between a server and a client using sockets.
4.  **Serialization:** Implemented logic to flatten complex graph structures into binary buffers which can be sent from the client and read from the server.
5.  **Signal Handling:** Solved a blocking `accept()` issue on WSL (Windows Subsystem for Linux), which caused `CTRL+C` to not emit `SIGINT`, by implementing `select()` with timeouts, allowing the server to catch `SIGINT` and shut down cleanly.
6.  **Integration:** Linked the logic library with the server network layer and configured the build steps in the `Makefile`. First successful remote run of Yen's algorithm.
7.  **Data Generation:** Used AI tools to generate valid and massive test datasets for benchmarking. In most cases, human fixes were required, limiting the number and size of the datasets.
8.  **Threadpool Implementation & Parallelisation:** Built a reusable thread pool class with task queues and worker synchronization (`std::mutex`, `std::condition_variable`). Integrated it in the server to handle multiple clients simultaneously. Parallelised Yen's algorithm by moving the inner loop into another thread pool.
9. **Optimization:** Tuned thread limits (e.g., 4 users $\times$ 4 threads) to prevent context switching overhead and exposed control to the client. Fixed numerous bugs and implemented the Selector pattern and minor optimizations.
10. **Benchmarking:** Implemented high-precision `std::chrono` timing (microseconds) to measure performance gains.
10. **Refactoring & README:** Wrote the README documentation and refactored the code by extracting common code into headers.


## How to Run

### Prerequisites
* **OS:** Ubuntu 20.04+ (or WSL2)
* **Compiler:** `g++` (supporting C++17 or higher)
* **Build System:** `make`

### Build & Execution
**1. Clone the repo and compile using the Makefile:**

```bash
$ git clone https://github.com/Eko199/Yens-Algorithm-over-Network
$ cd Yens-Algorithm-over-Network
$ make
```

**2. Start the Server:**
It listens on port **4095** and handles requests.
```bash
$ bin/server.out
```

**3. Start the Client:**
Open another terminal and run the client binary.
```bash
$ bin/client.out
```
Test files can be used as input.
```bash
$ bin/client.out < test/graph1.txt
```

**At the end, you can clean the binaries:**
```bash
$ make clean
```