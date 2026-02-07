#!/usr/bin/env python3
"""
# Values manually copied from results/consolidated_results.csv
# as per PA02 requirement (no CSV reading allowed)
"""

import matplotlib.pyplot as plt

# Thread counts tested
thread_counts = [1, 2, 4, 8]
message_sizes = [512, 1024, 2048, 4096]

# =============================================================================
# A1: Two-Copy Implementation - Latency (µs)
# Format: [t1, t2, t4, t8] for each message size
# =============================================================================
a1_latency_512 = [2.55, 1.72, 1.18, 1.02]
a1_latency_1024 = [3.00, 2.05, 1.44, 1.35]
a1_latency_2048 = [4.67, 3.26, 2.03, 2.38]
a1_latency_4096 = [7.42, 4.63, 2.90, 4.64]

# =============================================================================
# A2: One-Copy Implementation - Latency (µs)
# =============================================================================
a2_latency_512 = [1.06, 0.67, 0.56, 0.66]
a2_latency_1024 = [1.74, 1.00, 1.18, 1.24]
a2_latency_2048 = [2.51, 1.64, 2.68, 2.44]
a2_latency_4096 = [4.71, 3.02, 4.33, 4.89]

# =============================================================================
# A3: Zero-Copy Implementation - Latency (µs)
# =============================================================================
a3_latency_512 = [1.38, 0.83, 0.63, 0.73]
a3_latency_1024 = [2.07, 1.36, 1.07, 1.43]
a3_latency_2048 = [3.95, 3.05, 3.04, 2.78]
a3_latency_4096 = [6.71, 4.07, 3.72, 5.40]

# Create figure with 3 subplots (one per implementation)
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# Plot A1: Two-Copy
ax1 = axes[0]
ax1.plot(thread_counts, a1_latency_512, 'o-', linewidth=2, markersize=8, label='512 bytes')
ax1.plot(thread_counts, a1_latency_1024, 's-', linewidth=2, markersize=8, label='1024 bytes')
ax1.plot(thread_counts, a1_latency_2048, '^-', linewidth=2, markersize=8, label='2048 bytes')
ax1.plot(thread_counts, a1_latency_4096, 'd-', linewidth=2, markersize=8, label='4096 bytes')
ax1.set_xlabel('Number of Threads', fontsize=12, fontweight='bold')
ax1.set_ylabel('Latency (µs)', fontsize=12, fontweight='bold')
ax1.set_title('A1: Two-Copy (send/recv)', fontsize=13, fontweight='bold')
ax1.legend(loc='upper right')
ax1.grid(True, alpha=0.3, linestyle='--')
ax1.set_xticks(thread_counts)

# Plot A2: One-Copy
ax2 = axes[1]
ax2.plot(thread_counts, a2_latency_512, 'o-', linewidth=2, markersize=8, label='512 bytes')
ax2.plot(thread_counts, a2_latency_1024, 's-', linewidth=2, markersize=8, label='1024 bytes')
ax2.plot(thread_counts, a2_latency_2048, '^-', linewidth=2, markersize=8, label='2048 bytes')
ax2.plot(thread_counts, a2_latency_4096, 'd-', linewidth=2, markersize=8, label='4096 bytes')
ax2.set_xlabel('Number of Threads', fontsize=12, fontweight='bold')
ax2.set_ylabel('Latency (µs)', fontsize=12, fontweight='bold')
ax2.set_title('A2: One-Copy (sendmsg + iovec)', fontsize=13, fontweight='bold')
ax2.legend(loc='upper right')
ax2.grid(True, alpha=0.3, linestyle='--')
ax2.set_xticks(thread_counts)

# Plot A3: Zero-Copy
ax3 = axes[2]
ax3.plot(thread_counts, a3_latency_512, 'o-', linewidth=2, markersize=8, label='512 bytes')
ax3.plot(thread_counts, a3_latency_1024, 's-', linewidth=2, markersize=8, label='1024 bytes')
ax3.plot(thread_counts, a3_latency_2048, '^-', linewidth=2, markersize=8, label='2048 bytes')
ax3.plot(thread_counts, a3_latency_4096, 'd-', linewidth=2, markersize=8, label='4096 bytes')
ax3.set_xlabel('Number of Threads', fontsize=12, fontweight='bold')
ax3.set_ylabel('Latency (µs)', fontsize=12, fontweight='bold')
ax3.set_title('A3: Zero-Copy (MSG_ZEROCOPY)', fontsize=13, fontweight='bold')
ax3.legend(loc='upper right')
ax3.grid(True, alpha=0.3, linestyle='--')
ax3.set_xticks(thread_counts)

plt.suptitle('Message Latency vs Thread Count\nRoll: MT25190 | System: Linux | Duration: 30s per experiment',
             fontsize=14, fontweight='bold', y=1.02)

plt.tight_layout()
plt.savefig('MT25190_Latency_vs_ThreadCount.png', dpi=300, bbox_inches='tight')
print("Plot saved: MT25190_Latency_vs_ThreadCount.png")
plt.show()
