#!/usr/bin/env python3
"""
# Values manually copied from results/consolidated_results.csv
# as per PA02 requirement (no CSV reading allowed)
"""

import matplotlib.pyplot as plt

# Message sizes tested
message_sizes = [512, 1024, 2048, 4096]
thread_counts = [1, 2, 4, 8]

# =============================================================================
# A1: Two-Copy Implementation - Throughput (Gbps)
# =============================================================================
a1_throughput_t1 = [12.867482, 21.881606, 28.062282, 35.343017]
a1_throughput_t2 = [19.032230, 31.959941, 40.253261, 56.595446]
a1_throughput_t4 = [27.882105, 45.575520, 64.636987, 90.459078]
a1_throughput_t8 = [32.031218, 48.429173, 55.156315, 56.556364]

# =============================================================================
# A2: One-Copy Implementation - Throughput (Gbps)
# =============================================================================
a2_throughput_t1 = [31.053073, 37.572633, 52.321959, 55.592369]
a2_throughput_t2 = [49.133742, 65.505499, 80.000589, 86.799721]
a2_throughput_t4 = [58.083719, 55.565918, 48.960052, 60.226676]
a2_throughput_t8 = [49.283623, 52.768007, 53.658288, 53.635135]

# =============================================================================
# A3: Zero-Copy Implementation - Throughput (Gbps)
# =============================================================================
a3_throughput_t1 = [23.670250, 31.623075, 33.104753, 38.991085]
a3_throughput_t2 = [39.606024, 48.312073, 42.772196, 64.037822]
a3_throughput_t4 = [51.888326, 60.970831, 42.899810, 69.858118]
a3_throughput_t8 = [44.598725, 45.699746, 47.142679, 48.545002]

# Create figure with 3 subplots (one per implementation)
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# Plot A1: Two-Copy
ax1 = axes[0]
ax1.plot(message_sizes, a1_throughput_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax1.plot(message_sizes, a1_throughput_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax1.plot(message_sizes, a1_throughput_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax1.plot(message_sizes, a1_throughput_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax1.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax1.set_ylabel('Throughput (Gbps)', fontsize=12, fontweight='bold')
ax1.set_title('A1: Two-Copy (send/recv)', fontsize=13, fontweight='bold')
ax1.legend(loc='upper left')
ax1.grid(True, alpha=0.3, linestyle='--')
ax1.set_xscale('log', base=2)
ax1.set_xticks(message_sizes)
ax1.set_xticklabels([str(x) for x in message_sizes])

# Plot A2: One-Copy
ax2 = axes[1]
ax2.plot(message_sizes, a2_throughput_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax2.plot(message_sizes, a2_throughput_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax2.plot(message_sizes, a2_throughput_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax2.plot(message_sizes, a2_throughput_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax2.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax2.set_ylabel('Throughput (Gbps)', fontsize=12, fontweight='bold')
ax2.set_title('A2: One-Copy (sendmsg + iovec)', fontsize=13, fontweight='bold')
ax2.legend(loc='upper left')
ax2.grid(True, alpha=0.3, linestyle='--')
ax2.set_xscale('log', base=2)
ax2.set_xticks(message_sizes)
ax2.set_xticklabels([str(x) for x in message_sizes])

# Plot A3: Zero-Copy
ax3 = axes[2]
ax3.plot(message_sizes, a3_throughput_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax3.plot(message_sizes, a3_throughput_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax3.plot(message_sizes, a3_throughput_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax3.plot(message_sizes, a3_throughput_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax3.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax3.set_ylabel('Throughput (Gbps)', fontsize=12, fontweight='bold')
ax3.set_title('A3: Zero-Copy (MSG_ZEROCOPY)', fontsize=13, fontweight='bold')
ax3.legend(loc='upper left')
ax3.grid(True, alpha=0.3, linestyle='--')
ax3.set_xscale('log', base=2)
ax3.set_xticks(message_sizes)
ax3.set_xticklabels([str(x) for x in message_sizes])

plt.suptitle('Network I/O Throughput vs Message Size\nRoll: MT25190 | System: Linux | Duration: 30s per experiment',
             fontsize=14, fontweight='bold', y=1.02)

plt.tight_layout()
plt.savefig('MT25190_Throughput_vs_MessageSize.png', dpi=300, bbox_inches='tight')
print("Plot saved: MT25190_Throughput_vs_MessageSize.png")
plt.show()
