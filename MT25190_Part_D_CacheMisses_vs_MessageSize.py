#!/usr/bin/env python3
"""
# Values manually copied from results/consolidated_results.csv
# as per PA02 requirement (no CSV reading allowed)
# Using LLCMisses column (LLC-load-misses from perf)
"""

import matplotlib.pyplot as plt

# Message sizes tested
message_sizes = [512, 1024, 2048, 4096]
thread_counts = [1, 2, 4, 8]

# =============================================================================
# A1: Two-Copy Implementation - LLC Misses (raw counts)
# Format: [512, 1024, 2048, 4096] for each thread count
# =============================================================================
a1_llc_t1 = [1182965, 1650106, 38389687, 202979544]
a1_llc_t2 = [4553536, 5806962, 151830844, 241303311]
a1_llc_t4 = [15493195, 54462383, 474399924, 277005689]
a1_llc_t8 = [60807549, 430487024, 2037412624, 2504800793]

# =============================================================================
# A2: One-Copy Implementation - LLC Misses
# =============================================================================
a2_llc_t1 = [9077044, 40893048, 37951639, 34360883]
a2_llc_t2 = [16695155, 59978462, 91732892, 48207474]
a2_llc_t4 = [336230420, 1524378406, 2313540259, 2327336042]
a2_llc_t8 = [731889262, 1694690824, 2328728464, 2497613019]

# =============================================================================
# A3: Zero-Copy Implementation - LLC Misses
# =============================================================================
a3_llc_t1 = [8446393, 31250748, 66533862, 32318453]
a3_llc_t2 = [35742267, 118117234, 508736169, 103599845]
a3_llc_t4 = [944055814, 672884060, 1946458979, 1731823961]
a3_llc_t8 = [2226755894, 2926282126, 2961186130, 3276203920]

# Convert to millions for readability
def to_millions(arr):
    return [x / 1e6 for x in arr]

# Create figure with 3 subplots (one per implementation)
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# Plot A1: Two-Copy
ax1 = axes[0]
ax1.plot(message_sizes, to_millions(a1_llc_t1), 'o-', linewidth=2, markersize=8, label='1 Thread')
ax1.plot(message_sizes, to_millions(a1_llc_t2), 's-', linewidth=2, markersize=8, label='2 Threads')
ax1.plot(message_sizes, to_millions(a1_llc_t4), '^-', linewidth=2, markersize=8, label='4 Threads')
ax1.plot(message_sizes, to_millions(a1_llc_t8), 'd-', linewidth=2, markersize=8, label='8 Threads')
ax1.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax1.set_ylabel('LLC Misses (millions)', fontsize=12, fontweight='bold')
ax1.set_title('A1: Two-Copy (send/recv)', fontsize=13, fontweight='bold')
ax1.legend(loc='upper left')
ax1.grid(True, alpha=0.3, linestyle='--')
ax1.set_xscale('log', base=2)
ax1.set_xticks(message_sizes)
ax1.set_xticklabels([str(x) for x in message_sizes])

# Plot A2: One-Copy
ax2 = axes[1]
ax2.plot(message_sizes, to_millions(a2_llc_t1), 'o-', linewidth=2, markersize=8, label='1 Thread')
ax2.plot(message_sizes, to_millions(a2_llc_t2), 's-', linewidth=2, markersize=8, label='2 Threads')
ax2.plot(message_sizes, to_millions(a2_llc_t4), '^-', linewidth=2, markersize=8, label='4 Threads')
ax2.plot(message_sizes, to_millions(a2_llc_t8), 'd-', linewidth=2, markersize=8, label='8 Threads')
ax2.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax2.set_ylabel('LLC Misses (millions)', fontsize=12, fontweight='bold')
ax2.set_title('A2: One-Copy (sendmsg + iovec)', fontsize=13, fontweight='bold')
ax2.legend(loc='upper left')
ax2.grid(True, alpha=0.3, linestyle='--')
ax2.set_xscale('log', base=2)
ax2.set_xticks(message_sizes)
ax2.set_xticklabels([str(x) for x in message_sizes])

# Plot A3: Zero-Copy
ax3 = axes[2]
ax3.plot(message_sizes, to_millions(a3_llc_t1), 'o-', linewidth=2, markersize=8, label='1 Thread')
ax3.plot(message_sizes, to_millions(a3_llc_t2), 's-', linewidth=2, markersize=8, label='2 Threads')
ax3.plot(message_sizes, to_millions(a3_llc_t4), '^-', linewidth=2, markersize=8, label='4 Threads')
ax3.plot(message_sizes, to_millions(a3_llc_t8), 'd-', linewidth=2, markersize=8, label='8 Threads')
ax3.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax3.set_ylabel('LLC Misses (millions)', fontsize=12, fontweight='bold')
ax3.set_title('A3: Zero-Copy (MSG_ZEROCOPY)', fontsize=13, fontweight='bold')
ax3.legend(loc='upper left')
ax3.grid(True, alpha=0.3, linestyle='--')
ax3.set_xscale('log', base=2)
ax3.set_xticks(message_sizes)
ax3.set_xticklabels([str(x) for x in message_sizes])

plt.suptitle('LLC Cache Misses vs Message Size\nRoll: MT25190 | System: Linux | Duration: 30s per experiment',
             fontsize=14, fontweight='bold', y=1.02)

plt.tight_layout()
plt.savefig('MT25190_CacheMisses_vs_MessageSize.png', dpi=300, bbox_inches='tight')
print("Plot saved: MT25190_CacheMisses_vs_MessageSize.png")
plt.show()
