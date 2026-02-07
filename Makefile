# Makefile for PA02: Network I/O Primitives Analysis
# Roll Number: MT25190
# Course: CSE638 - Graduate Systems

CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread
LDFLAGS = -pthread

# Source files
A1_SERVER_SRC = MT25190_Part_A1_Server.c
A1_CLIENT_SRC = MT25190_Part_A1_Client.c
A2_SERVER_SRC = MT25190_Part_A2_Server.c
A2_CLIENT_SRC = MT25190_Part_A2_Client.c
A3_SERVER_SRC = MT25190_Part_A3_Server.c
A3_CLIENT_SRC = MT25190_Part_A3_Client.c

# Binary names
A1_SERVER_BIN = MT25190_Part_A1_Server
A1_CLIENT_BIN = MT25190_Part_A1_Client
A2_SERVER_BIN = MT25190_Part_A2_Server
A2_CLIENT_BIN = MT25190_Part_A2_Client
A3_SERVER_BIN = MT25190_Part_A3_Server
A3_CLIENT_BIN = MT25190_Part_A3_Client

# All targets
ALL_BINS = $(A1_SERVER_BIN) $(A1_CLIENT_BIN) \
           $(A2_SERVER_BIN) $(A2_CLIENT_BIN) \
           $(A3_SERVER_BIN) $(A3_CLIENT_BIN)

.PHONY: all clean help run_experiments

# Default target
all: $(ALL_BINS)
	@echo ""
	@echo "=== Compilation Complete ==="
	@echo "Roll Number: MT25190"
	@echo ""
	@echo "Built binaries:"
	@ls -lh $(ALL_BINS)
	@echo ""

# Part A1: Two-Copy Implementation
$(A1_SERVER_BIN): $(A1_SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A1_CLIENT_BIN): $(A1_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Part A2: One-Copy Implementation
$(A2_SERVER_BIN): $(A2_SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A2_CLIENT_BIN): $(A2_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Part A3: Zero-Copy Implementation
$(A3_SERVER_BIN): $(A3_SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(A3_CLIENT_BIN): $(A3_CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(ALL_BINS)
	rm -f *.o
	rm -rf results/
	@echo "Clean complete."

# Run automated experiments
run_experiments: all
	@echo "Running automated experiments..."
	@chmod +x MT25190_Part_C.sh
	@./MT25190_Part_C.sh

# Generate plots
plots:
	@echo "Generating plots..."
	@python3 MT25190_Part_D_Throughput_vs_MessageSize.py
	@python3 MT25190_Part_D_Latency_vs_ThreadCount.py
	@python3 MT25190_Part_D_CacheMisses_vs_MessageSize.py
	@python3 MT25190_Part_D_CyclesPerByte.py
	@echo "Plots generated."

# Help target
help:
	@echo "=== PA02 Makefile Help ==="
	@echo "Roll Number: MT25190"
	@echo ""
	@echo "Available targets:"
	@echo "  make              - Build all implementations"
	@echo "  make all          - Same as 'make'"
	@echo "  make clean        - Remove all binaries and results"
	@echo "  make run_experiments - Build and run all experiments with perf"
	@echo "  make plots        - Generate all visualization plots"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Individual builds:"
	@echo "  make $(A1_SERVER_BIN)"
	@echo "  make $(A1_CLIENT_BIN)"
	@echo "  make $(A2_SERVER_BIN)"
	@echo "  make $(A2_CLIENT_BIN)"
	@echo "  make $(A3_SERVER_BIN)"
	@echo "  make $(A3_CLIENT_BIN)"
	@echo ""
	@echo "Usage example:"
	@echo "  1. make clean"
	@echo "  2. make all"
	@echo "  3. make run_experiments"
	@echo "  4. make plots"
	@echo ""
