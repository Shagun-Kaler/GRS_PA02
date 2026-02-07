#!/bin/bash
# Compiles, runs experiments with perf, generates CSV results

# NOTE: Removed 'set -e' to handle server termination gracefully

echo "=== PA02 Automated Experiments ==="
echo "Roll Number: MT25190"
echo ""

# Configuration
# QUICK TEST: Set QUICK_TEST=1 for fast testing (2 sizes × 2 threads)
# FINAL RUN: Set QUICK_TEST=0 for full coverage (4 sizes × 4 threads)
QUICK_TEST=0  # Set to 1 for quick testing, 0 for full experiments

if [ "$QUICK_TEST" = "1" ]; then
    MESSAGE_SIZES=(512 1024)
    THREAD_COUNTS=(1 2)
    DURATION=3
else
    MESSAGE_SIZES=(512 1024 2048 4096)  # All message sizes for full coverage
    THREAD_COUNTS=(1 2 4 8)             # All thread counts for full coverage
    DURATION=30                         # 30 seconds per experiment
fi

RESULTS_DIR="results"
SERVER_IP="127.0.0.1"  # PA02: Localhost for single-machine testing
PERF_EVENTS="cpu-cycles,cache-misses,L1-dcache-load-misses,LLC-load-misses,context-switches"

# Clean previous results and recreate directory
# NOTE: results/ must exist before perf stat writes output files
echo "Cleaning previous results..."
rm -rf ${RESULTS_DIR}
mkdir -p ${RESULTS_DIR}

# Create single consolidated CSV file with header
# Added ThroughputGbps, LatencyUs, TotalBytes from client METRICS output for Part D plots
CONSOLIDATED_CSV="${RESULTS_DIR}/MT25190_Part_C_results.csv"
echo "Implementation,MessageSize,Threads,CPUCycles,CacheMisses,L1Misses,LLCMisses,ContextSwitches,TimeElapsed,ThroughputGbps,LatencyUs,TotalBytes" > "${CONSOLIDATED_CSV}"

# FIX: Check perf permissions before running experiments
PERF_PARANOID=$(cat /proc/sys/kernel/perf_event_paranoid 2>/dev/null || echo "unknown")
if [ "$PERF_PARANOID" != "-1" ] && [ "$PERF_PARANOID" != "unknown" ]; then
    echo "WARNING: perf_event_paranoid is set to ${PERF_PARANOID}"
    echo "Experiments may fail. Run: sudo sysctl -w kernel.perf_event_paranoid=-1"
    echo ""
fi

# Compile all implementations
echo "Compiling implementations..."
make clean
make all

echo "Compilation complete."
echo ""

# Function to run experiment with perf
run_experiment() {
    local impl=$1      # A1, A2, A3
    local msg_size=$2
    local threads=$3
    local port=$4
    
    local server_bin="MT25190_Part_${impl}_Server"
    local client_bin="MT25190_Part_${impl}_Client"
    local perf_file="${RESULTS_DIR}/${impl}_msg${msg_size}_t${threads}_perf.txt"
    local metrics_file="${RESULTS_DIR}/${impl}_msg${msg_size}_t${threads}_metrics.txt"
    
    # FIX: Ensure results directory exists before perf writes output
    mkdir -p "${RESULTS_DIR}"
    
    echo "Running: ${impl} | MsgSize=${msg_size} | Threads=${threads} | Port=${port}"
    
    # Start server in background with: <port> <message_size> <num_threads>
    # PA02 requirement: Port must be passed explicitly
    ./${server_bin} ${port} ${msg_size} ${threads} > /dev/null 2>&1 &
    SERVER_PID=$!
    sleep 1  # Let server initialize (quick test)
    
    # Run client with perf profiling: <server_ip> <port> <message_size> <num_threads> <duration>
    # PA02 requirement: All parameters passed explicitly for automation
    # NOTE: perf stat writes to stderr, client METRICS writes to stdout
    # FIX: Capture stdout to metrics file for application-level data
    perf stat -e ${PERF_EVENTS} \
        ./${client_bin} ${SERVER_IP} ${port} ${msg_size} ${threads} ${DURATION} \
        > "${metrics_file}" 2> "${perf_file}"
    
    # Kill server
    kill ${SERVER_PID} 2>/dev/null || true
    wait ${SERVER_PID} 2>/dev/null || true
    
    # Parse perf output to CSV
    # NOTE: Only parse if perf output file was successfully created
    # FIX: Check file exists AND is not empty
    if [ -f "${perf_file}" ] && [ -s "${perf_file}" ]; then
        # FIX: Write directly to consolidated CSV (single file for all results)
        # Pass metrics file for application-level data extraction
        parse_perf_to_csv ${perf_file} ${metrics_file} ${CONSOLIDATED_CSV} ${impl} ${msg_size} ${threads}
    else
        echo "WARNING: Perf output file not created or empty: ${perf_file}"
    fi
    
    sleep 1  # Cooldown between experiments (quick test)
}

# Parse perf output to CSV format
parse_perf_to_csv() {
    local perf_file=$1
    local metrics_file=$2
    local csv_file=$3
    local impl=$4
    local msg_size=$5
    local threads=$6
    
    # Extract metrics from perf output (handle hybrid CPU architectures)
    # Sum values from all CPU types (atom/core) and remove commas/angle brackets
    cpu_cycles=$(grep -E "(cpu-cycles|cpu_atom/cpu-cycles|cpu_core/cpu-cycles)" ${perf_file} | awk '{print $1}' | grep -v '<' | tr -d ',' | awk '{sum+=$1} END {print sum}')
    cache_misses=$(grep -E "(cache-misses|cpu_atom/cache-misses|cpu_core/cache-misses)" ${perf_file} | awk '{print $1}' | grep -v '<' | tr -d ',' | awk '{sum+=$1} END {print sum}')
    llc_misses=$(grep -E "(LLC-load-misses|cpu_atom/LLC-load-misses|cpu_core/LLC-load-misses)" ${perf_file} | awk '{print $1}' | grep -v '<' | tr -d ',' | awk '{sum+=$1} END {print sum}')
    l1_misses=$(grep -E "(L1-dcache-load-misses|cpu_atom/L1-dcache-load-misses|cpu_core/L1-dcache-load-misses)" ${perf_file} | awk '{print $1}' | grep -v '<' | tr -d ',' | awk '{sum+=$1} END {print sum}')
    ctx_switches=$(grep "context-switches" ${perf_file} | awk '{print $1}' | grep -v '<' | tr -d ',' | head -1)
    time_elapsed=$(grep "seconds time elapsed" ${perf_file} | awk '{print $1}' | head -1)
    
    # FIX: Extract application-level metrics from client output
    # Client prints: METRICS throughput_gbps=X latency_us=Y bytes=Z
    throughput_gbps=$(grep "METRICS" ${metrics_file} 2>/dev/null | sed -n 's/.*throughput_gbps=\([^ ]*\).*/\1/p' | head -1)
    latency_us=$(grep "METRICS" ${metrics_file} 2>/dev/null | sed -n 's/.*latency_us=\([^ ]*\).*/\1/p' | head -1)
    total_bytes=$(grep "METRICS" ${metrics_file} 2>/dev/null | sed -n 's/.*bytes=\([^ ]*\).*/\1/p' | head -1)
    
    # Handle missing or empty values
    cpu_cycles=${cpu_cycles:-0}
    cache_misses=${cache_misses:-0}
    l1_misses=${l1_misses:-0}
    llc_misses=${llc_misses:-0}
    ctx_switches=${ctx_switches:-0}
    time_elapsed=${time_elapsed:-0}
    throughput_gbps=${throughput_gbps:-0}
    latency_us=${latency_us:-0}
    total_bytes=${total_bytes:-0}
    
    # FIX: Header already exists in consolidated CSV, just append data
    # Append data with application metrics
    echo "${impl},${msg_size},${threads},${cpu_cycles},${cache_misses},${l1_misses},${llc_misses},${ctx_switches},${time_elapsed},${throughput_gbps},${latency_us},${total_bytes}" >> ${csv_file}
}

# Run experiments for all combinations
for impl in A1 A2 A3; do
    # Set port based on implementation
    if [ "$impl" = "A1" ]; then
        port=8080
    elif [ "$impl" = "A2" ]; then
        port=8081
    else
        port=8082
    fi
    
    echo ""
    echo "=== Testing Implementation ${impl} ==="
    
    for msg_size in "${MESSAGE_SIZES[@]}"; do
        for threads in "${THREAD_COUNTS[@]}"; do
            run_experiment ${impl} ${msg_size} ${threads} ${port}
        done
    done
done

# FIX: No consolidation needed - already writing to single CSV file
echo ""
echo "=== Experiment Complete ==="
echo "Results saved in ${RESULTS_DIR}/"
echo "Consolidated results: ${CONSOLIDATED_CSV}"
echo ""
echo "Key files generated:"
ls -lh ${CONSOLIDATED_CSV}
echo ""
echo "Perf output files:"
ls ${RESULTS_DIR}/*.txt | wc -l
echo "perf files in results/"
echo ""
echo "To generate plots, run:"
echo "  python3 MT25190_Part_D_Throughput_vs_MessageSize.py"
echo "  python3 MT25190_Part_D_Latency_vs_ThreadCount.py"
echo "  python3 MT25190_Part_D_CacheMisses_vs_MessageSize.py"
echo "  python3 MT25190_Part_D_CyclesPerByte.py"
