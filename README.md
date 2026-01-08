# Memory & Cache Management Simulator

A comprehensive simulation tool designed to demonstrate the interactions between Physical Memory Management (Allocation) and CPU Cache Hierarchies. This project allows users to visualize memory layout, analyze fragmentation, and test various cache replacement policies in a controlled environment.

## 🚀 Getting Started

### Prerequisites
*   **Operating System:** Windows (or Linux/macOS with appropriate build tools).
*   **Compiler:** G++ (Supporting C++17 standard).
*   **Build Tool:** Make (specifically `mingw32-make` on Windows or `make` on Linux/Mac).

### Compilation
The project utilizes a `Makefile` for easy compilation.

1.  Open a terminal in the project root directory.
2.  Run the make command:
    *   **Windows:**
        ```bash
        mingw32-make
        ```
    *   **Linux/Mac:**
        ```bash
        make
        ```
3.  To clean and rebuild from scratch:
    ```bash
    mingw32-make rebuild  # or 'make rebuild'
    ```

### Execution
Run the simulator using the make command:
```bash
mingw32-make run  # or 'make run'
```
Or execute the binary directly:
```bash
./bin/MemoryManagementSimulator.exe
```

---

## 📂 Project Structure

*   **`allocator/`**: Content related to Physical Memory Management.
    *   `MemoryManager.h/cpp`: Implements allocation strategies (First/Best/Worst Fit) and memory tracking.
*   **`cache/`**: Content related to CPU Cache Simulation.
    *   `CacheSimulator.h/cpp`: Implements L1/L2 hierarchy and replacement policies (FIFO, LRU, LFU).
*   **`stats/`**: Statistics tracking.
    *   `StatsManager.h/cpp`: Collects and aggregates metrics for reporting.
*   **`main.cpp`**: The CLI entry point handling user input and command parsing.
*   **`Makefile`**: Build configuration script.

---

## 💻 Supported Commands

The simulator operates via a Command Line Interface (CLI). Below are the available commands:

| Command | Description | Example |
| :--- | :--- | :--- |
| `init memory <size>` | Initialize Physical RAM with a specific size (bytes). | `init memory 1024` |
| `init cache <p1>...` | Initialize Cache Hierarchy (L1 Size, Block Size, Assoc, L2 Size...). | `init cache 64 8 2 256 16 4` |
| `set allocator <strat>` | Set memory allocation strategy. Options: `first_fit`, `best_fit`, `worst_fit`. | `set allocator best_fit` |
| `set cache_policy <pol>`| Set cache replacement policy. Options: `fifo`, `lru`, `lfu`. | `set cache_policy lru` |
| `malloc <size>` | Allocate a block of memory of size `<size>`. | `malloc 128` |
| `free <id/addr>` | Free a block by its ID or 0xAddress. | `free 1` or `free 0x48` |
| `access <addr>` | Simulate a memory access to a **Physical Address**. | `access 0x10` |
| `dump memory` | Display the current status of all physical memory blocks. | `dump memory` |
| `stats` | Show detailed statistics for memory and cache. | `stats` |
| `exit` | Quit the simulator. | `exit` |

### Allocation Strategies
1.  **First Fit (`first_fit`):** Allocates the first free block that is large enough.
2.  **Best Fit (`best_fit`):** Allocates the smallest free block that fits the request (minimizes wasted space).
3.  **Worst Fit (`worst_fit`):** Allocates the largest free block available (leaves large gaps).

### Cache Replacement Policies
1.  **FIFO (`fifo`):** First-In, First-Out. Evicts the oldest block loaded into the set.
2.  **LRU (`lru`):** Least Recently Used. Evicts the block that hasn't been accessed for the longest time.
3.  **LFU (`lfu`):** Least Frequently Used. Evicts the block with the fewest accesses.

---

## 📊 Assumptions & Design Choices

### Physical Memory
*   **Contiguous Allocation:** The memory manager allocates contiguous blocks of physical memory.
*   **Block Headers:** Each allocated block incurs a `BlockHeader` overhead (internally managed). This means a user request for 100 bytes consumes 100 + `sizeof(Header)` bytes of actual memory.
*   **Coalescing:** When a block is freed, the allocator automatically merges it with adjacent free blocks to reduce fragmentation.
*   **Adressing:** Addresses start at `0x00000000` and are strictly Physical Addresses.

### Cache Hierarchy
*   **Hierarchy:** Standard 2-Level (L1, L2) hierarchy.
*   **Inclusion Logic:**
    *   **L1 Miss, L2 Miss:** Data is loaded from Main Memory -> L2 -> L1.
    *   **L1 Miss, L2 Hit:** Data is loaded from L2 -> L1.
    *   **L1 Hit:** Immediate access.
*   **Replacement Scopes:** Eviction policies (FIFO/LRU/LFU) operate per *Set* (Set Associative Cache).
*   **Cold Start:** Caches start empty and valid bits are `false` until the first load.
*   **Default Configuration:** If `init cache` is not explicitly called, `init memory` sets up a default hierarchy:
    *   **L1:** 16 KB Size, 64 B Block, 4-way Set Associative.
    *   **L2:** 64 KB Size, 64 B Block, 8-way Set Associative.


---

## 📘 Design Document

### 1. Memory Layout & Assumptions
*   **Physical Address Space:** The simulator models a flat, zero-based physical address space (0x0 to `MEMORY_SIZE - 1`).
*   **Memory Management:** The system manages memory using a `BlockHeader` linked list embedded directly in the simulated RAM.
    *   **Structure:** `[Header | User Data ] [Header | User Data ] ...`
    *   **Overhead:** Each allocation consumes `sizeof(BlockHeader)` (typically 32 bytes) + requested user size.
    *   **Coalescing:** Deallocation (Free) triggers an immediate merge of the freed block with adjacent free blocks (previous and next), preventing fragmentation from small holes.

### 2. Allocation Strategy Implementation
The `MemoryManager` supports three classic algorithms for finding free blocks:
*   **First Fit:** Scans the free list linearly and selects the *first* block that satisfies `block.size >= requested_size`. Fast but may cause fragmentation at the start of memory.
*   **Best Fit:** Scans the *entire* free list to find the block closest in size to the request. Minimizes wasted space (Internal Fragmentation) but is slower (O(N)).
*   **Worst Fit:** Scans the entire list to find the *largest* available block. Intended to leave large holes for future allocations, but often leads to poor utilization.

### 3. Cache Simulation Design
*   **Hierarchy:** A 2-level simulation (L1 and L2).
*   **Architecture:**
    *   **Inclusive-like Behavior:** A miss in L1 triggers an access to L2. If found in L2 (Hit), data is brought to L1. If missed in L2, data is fetched from Memory -> L2 -> L1.
    *   **Set Associative:** addresses map to specific sets based on `(Address >> BlockOffsetBits) & SetMask`.
*   **Replacement Policy:**
    *   **FIFO:** Maintains a `std::queue` of indices. The first block inserted is the first evicted.
    *   **LRU:** Uses a `global_time` counter. Each access updates the block's `last_access` timestamp. The block with the smallest timestamp in the set is evicted.
    *   **LFU:** Tracks `access_count`. The block with the lowest frequency is evicted.

### 4. Address Translation Flow
*   **Simplified Model:** This simulator operates purely on **Physical Addresses**.
    *   **No Virtual Memory:** There is no Virtual-to-Physical translation, Page Tables, or TLB simulation.
    *   **Flow:** User input `access 0x1A` -> `CacheSimulator::access(0x1A)` -> `L1 Set Index` -> `Tag Check` -> ...

### 5. Limitations & Simplifications
*   **Single Process:** The simulator assumes a single execution context; there is no process isolation or context switching.
*   **No Paging:** As VMM was removed, there is no disk swapping or page fault handling.
*   **Static Config:** Cache parameters (size/associativity) must be defined at initialization (`init cache`) and cannot be changed dynamically without a full reset.
*   **Data Content:** The simulator tracks *allocations* but does not store actual user data values (e.g., writing integers to memory). It simulates the *state* of memory, not the content.

---

## 📈 Statistics Output

When running the `stats` command, the simulator provides a comprehensive report:

### 1. Memory Allocation
*   **Total Allocations:** Count of `malloc` commands issued.
*   **Success/Failure:** Count of requests that fit vs. those that were rejected due to Out-Of-Memory (OOM).
*   **Success Rate:** `(Successful / Total) * 100`

### 2. Memory Usage
*   **Used Memory:** Total bytes currently allocated (User Data + Headers).
*   **Free Memory:** Total bytes currently available.
*   **Utilization:** `(Used Memory / Total Memory) * 100`

### 3. Fragmentation
*   **Internal Fragmentation:** Percentage of space wasted inside allocated blocks (overhead vs. requested size).
    ```math
    ((TotalAllocated - TotalRequested) / TotalAllocated) * 100
    ```
*   **External Fragmentation:** Percentage of free memory unusable for large allocations due to scattering.
    ```math
    ((TotalFree - LargestFreeBlock) / TotalMemory) * 100
    ```

### 4. Cache Statistics (Per Level: L1 & L2)
*   **Hits:** Number of accesses found in the cache.
*   **Misses:** Number of accesses not found.
*   **Hit Ratio:** `(Hits / (Hits + Misses)) * 100`

---

## 📝 Usage Example

**1. Initialize System**
```bash
# Initialize 4096 bytes (4KB) of RAM
init memory 4096

# Initialize Cache: 
# L1: 128B size, 16B blocks, 2-way associativity
# L2: 1024B size, 16B blocks, 4-way associativity
init cache 128 16 2 1024 16 4
```

**2. Allocate Memory**
```bash
set allocator best_fit
malloc 100  # Allocates Block 1
malloc 200  # Allocates Block 2
free 1      # Creates a hole
malloc 50   # Should fill part of the hole left by Block 1
```

**3. Simulating Access & Analyzing Stats**
```bash
# Access memory (Triggers Cache mechanics)
access 0x0   # Cold Miss -> Fill L2 -> Fill L1
access 0x0   # Hit L1

# Check fragmentation and usage
dump memory
stats
```
