# PA02: Analysis of Network I/O Primitives using perf

**Roll Number:** MT25190  
**Course:** CSE638 - Graduate Systems

## Project Overview

This project implements and analyzes three network I/O paradigms:
- **Part A1:** Two-Copy Implementation (send/recv)
- **Part A2:** One-Copy Implementation (sendmsg + iovec)
- **Part A3:** Zero-Copy Implementation (MSG_ZEROCOPY)

Each implementation includes client-server architecture with multithreading support, profiling with `perf`, and automated visualization.

## Directory Structure

```
MT25190_PA02/
├── MT25190_Part_A1_Server.c          # Two-copy TCP server (port 8080)
├── MT25190_Part_A1_Client.c          # Two-copy TCP client
├── MT25190_Part_A2_Server.c          # One-copy server with iovec (port 8081)
├── MT25190_Part_A2_Client.c          # One-copy client with recvmsg
├── MT25190_Part_A3_Server.c          # Zero-copy server with MSG_ZEROCOPY (port 8082)
├── MT25190_Part_A3_Client.c          # Zero-copy client
├── MT25190_Part_C_run_experiments_.sh # Automated experiment script
├── MT25190_Part_D_Throughput_vs_MessageSize.py
├── MT25190_Part_D_Latency_vs_ThreadCount.py
├── MT25190_Part_D_CacheMisses_vs_MessageSize.py
├── MT25190_Part_D_CyclesPerByte.py
├── Makefile                           # Build system
├── README.md                          # This file
└── MT25190_Part_C_results.csv                        # reults are stored in CSV


## Key Features

### Part A: Implementations

#### A1: Two-Copy (Baseline) - Port 8080
- Uses `send()` and `recv()` syscalls
- **Copy 1:** User space → Kernel socket buffer
- **Copy 2:** Kernel buffer → NIC (via DMA)
- Message structure with 8 dynamically allocated fields
- Signal handling for graceful shutdown (SIGINT, SIGTERM)
- Fully commented explaining each copy operation

#### A2: One-Copy (Optimized) - Port 8081
- Uses `sendmsg()` / `recvmsg()` with `struct iovec`
- Pre-registered page-aligned buffers (`aligned_alloc(4096, ...)`)
- Scatter-gather I/O eliminates intermediate buffer copies
- **Eliminates:** User → Kernel copy
- **Remaining:** Direct DMA from user buffers to NIC
- Comments explain kernel behavior and buffer management

#### A3: Zero-Copy (Advanced) - Port 8082
- Uses `MSG_ZEROCOPY` flag with `send()`
- Page pinning via `mlock()` for DMA-safe memory
- Drains completion notifications from `MSG_ERRQUEUE`
- ASCII diagram in comments showing data flow
- Explains page pinning, DMA descriptors, and completion notifications

### Part B: Profiling Integration
All implementations are designed to be profiled with:
```bash
perf stat -e cpu-cycles,cache-misses,L1-dcache-load-misses,LLC-load-misses,context-switches ./binary
```

### Part C: Automation
The bash script `MT25190_Part_C_run_experiments.sh`:
- Compiles all implementations via Makefile
- Runs experiments with varying message sizes (512, 1024, 2048, 4096)
- Varies thread counts (1, 2, 4, 8)
- Captures `perf` metrics and application-level throughput/latency
- Generates consolidated CSV with all results
- Supports `QUICK_TEST=1` mode for faster testing (2 sizes × 2 threads)
- Handles hybrid CPU architectures (sums metrics across CPU types)

### Part D: Visualization
Four Python plotting scripts with **hardcoded data** (values copied from MT25190_Part_C_results.csv):
1. **Throughput vs Message Size:** Shows performance scaling across implementations
2. **Latency vs Thread Count:** Analyzes contention effects
3. **Cache Misses vs Message Size:** Memory hierarchy impact
4. **Cycles per Byte:** CPU efficiency metric

Each plot includes:
- Proper axis labels with units
- Legend with thread counts
- System configuration annotation (Roll Number, Platform, Duration)
- Professional formatting with 300 DPI output

## Building and Running

### Prerequisites
```bash
# Install required tools (Linux)
sudo apt-get install build-essential linux-tools-common linux-tools-generic
sudo apt-get install python3 python3-matplotlib python3-numpy

# Enable perf for non-root users (optional)
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### Build All Implementations
```bash
make clean
make all
```

### Run Individual Tests
```bash
# Terminal 1: Start server (A1 example)
# Usage: ./server <port> <message_size> <num_threads>
./MT25190_Part_A1_Server 8080 1024 4

# Terminal 2: Run client
# Usage: ./client <server_ip> <port> <message_size> <num_threads> <duration_seconds>
./MT25190_Part_A1_Client 127.0.0.1 8080 1024 4 30

# A2 One-Copy (uses port 8081)
./MT25190_Part_A2_Server 8081 1024 4
./MT25190_Part_A2_Client 127.0.0.1 8081 1024 4 30

# A3 Zero-Copy (uses port 8082)
./MT25190_Part_A3_Server 8082 1024 4
./MT25190_Part_A3_Client 127.0.0.1 8082 1024 4 30
```

### Run Automated Experiments
```bash
make run_experiments
```
This will:
- Compile all code via Makefile
- Run 48 experiments (3 implementations × 4 message sizes × 4 thread counts)
- Capture perf metrics and application throughput/latency
- Generate consolidated CSV in `results/MT25190_Part_C_results.csv`
- Takes approximately 25-30 minutes (30 seconds per experiment)

**Quick Test Mode** (for debugging):
```bash
# Edit MT25190_Part_C_run_experiments.sh and set QUICK_TEST=1
# This runs only 12 experiments with 3-second duration each
```

### Generate Plots
```bash
make plots
```
Or run individually:
```bash
python3 MT25190_Part_D_Throughput_vs_MessageSize.py
python3 MT25190_Part_D_Latency_vs_ThreadCount.py
python3 MT25190_Part_D_CacheMisses_vs_MessageSize.py
python3 MT25190_Part_D_CyclesPerByte.py
```

**Generated Plot Files:**
- `MT25190_Throughput_vs_MessageSize.png`
- `MT25190_Latency_vs_ThreadCount.png`
- `MT25190_CacheMisses_vs_MessageSize.png`
- `MT25190_CyclesPerByte.png`

## Technical Details

### Message Structure
All implementations use 8-field messages:
- Each field is dynamically allocated (`malloc()` or `aligned_alloc()`)
- Field sizes configurable via command line
- Total message size per send = field_size × 8 bytes

### Threading Model
- **Server:** One pthread per client connection
- **Client:** Multiple pthreads connecting simultaneously
- Thread-safe implementation with proper synchronization
- Default run duration: 30 seconds per experiment

### Performance Metrics
Captured via `perf stat`:
- `cpu-cycles` - Total CPU cycles consumed
- `cache-misses` - Cache miss count
- `L1-dcache-load-misses` - L1 data cache misses
- `LLC-load-misses` - Last Level Cache misses
- `context-switches` - Number of context switches

Application-level metrics (from client output):
- `throughput_gbps` - Throughput in Gbps
- `latency_us` - Average latency in microseconds
- `bytes` - Total bytes transferred

### Copy Mechanisms Explained

**Two-Copy (A1):**
```
User Buffer → [memcpy] → Kernel sk_buff → [DMA] → NIC
              COPY 1                      COPY 2
```

**One-Copy (A2):**
```
User Buffer (page-aligned) → [DMA via scatter-gather] → NIC
                                    SINGLE COPY
```

**Zero-Copy (A3):**
```
User Buffer (mlock'd + page-aligned) → [Direct DMA] → NIC
                                       TRUE ZERO-COPY
Note: Requires MSG_ERRQUEUE completion notification handling
```

## Code Quality

### Comments
Every non-trivial line is commented, including:
- Copy operation identification (COPY 1, COPY 2)
- Kernel behavior explanation (tcp_sendmsg, sk_buff, DMA descriptors)
- Memory management details (malloc, aligned_alloc, mlock)
- Thread safety considerations
- PA02 requirement annotations

### Viva-Ready Features
- Clear variable names matching concepts (e.g., `send_message_twocopy`)
- Modular function design with detailed docstrings
- ASCII diagrams for zero-copy architecture
- Signal handling for graceful shutdown (SIGINT, SIGTERM)
- Error handling with `perror()` throughout

## Sample Results

Based on experimental runs (30 seconds each, localhost):

| Implementation | Peak Throughput (Gbps) | Best Config |
|---------------|----------------------|-------------|
| A1 Two-Copy   | ~90.5                | 4096B, 4 threads |
| A2 One-Copy   | ~87.0                | 4096B, 2 threads |
| A3 Zero-Copy  | ~69.9                | 4096B, 4 threads |

*Note: Zero-copy shows lower peak throughput on localhost due to MSG_ERRQUEUE overhead; benefits are more visible on real network interfaces.*

## System Configuration

- **OS:** Linux Ubuntu 24.04 LTS
- **Compiler:** 11.4.0
- **RAM:** 8-16 GB
- **CPU:** Intel/AMD x86_64 (specific model from system)
- **Network:** Loopback interface (localhost)


