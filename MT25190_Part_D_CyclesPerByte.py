#!/usr/bin/env python3
"""
# Values manually copied from results/consolidated_results.csv
# as per PA02 requirement (no CSV reading allowed)
# Computed as: CPUCycles / TotalBytes
"""

import matplotlib.pyplot as plt

# Message sizes tested
message_sizes = [512, 1024, 2048, 4096]
thread_counts = [1, 2, 4, 8]

# =============================================================================
# A1: Two-Copy Implementation - Cycles per Byte
# Computed from CSV: CPUCycles / TotalBytes
# Format: [512, 1024, 2048, 4096] for each thread count
# =============================================================================
# t1: 211972758136/48253067264, 209865235132/82056028160, 194443371119/105233563648, 192751235490/132536336384
a1_cpb_t1 = [4.39, 2.56, 1.85, 1.45]
# t2: 388916874569/71370866688, 368497953176/119849779200, 337956256754/150949756928, 327036825302/212232929280
a1_cpb_t2 = [5.45, 3.07, 2.24, 1.54]
# t4: 659712356290/104557899776, 621378449506/170908221440, 585842552895/242388746240, 545330766760/339221741568
a1_cpb_t4 = [6.31, 3.64, 2.42, 1.61]
# t8: 858911564665/120123695104, 790173513239/181609406464, 942951103264/206836269056, 1047858119940/212086521856
a1_cpb_t8 = [7.15, 4.35, 4.56, 4.94]

# =============================================================================
# A2: One-Copy Implementation - Cycles per Byte
# =============================================================================
# t1: 190851730358/116449026048, 163707030611/140897386496, 177415908932/196207362048, 158035962299/208471392256
a2_cpb_t1 = [1.64, 1.16, 0.90, 0.76]
# t2: 371372169984/184251539456, 357231665368/245645631488, 332554573474/300002312192, 303691406521/325498994688
a2_cpb_t2 = [2.02, 1.45, 1.11, 0.93]
# t4: 663679652696/217813970944, 732496403523/208372211712, 747554396518/183600250880, 710957329903/225850195968
a2_cpb_t4 = [3.05, 3.52, 4.07, 3.15]
# t8: 857823781113/184813616266, 934431950840/197880312565, 979882088948/201218627701, 1107195540797/201131848861
a2_cpb_t8 = [4.64, 4.72, 4.87, 5.50]

# =============================================================================
# A3: Zero-Copy Implementation - Cycles per Byte
# =============================================================================
# t1: 140600647661/88763437056, 150327361919/118586540032, 144546297574/124142829568, 138837854419/146216583168
a3_cpb_t1 = [1.58, 1.27, 1.16, 0.95]
# t2: 309968903000/148522598400, 280619437001/181170315264, 253569647207/160396083200, 273676432100/240141893632
a3_cpb_t2 = [2.09, 1.55, 1.58, 1.14]
# t4: 565993476664/194582167499, 470191312495/228641578955, 554197719906/161282653996, 569069013764/262818168832
a3_cpb_t4 = [2.91, 2.06, 3.44, 2.17]
# t8: 884767384962/167911235266, 1281235539363/171633622775, 990970229343/176817683314, 1373147392890/182043770297
a3_cpb_t8 = [5.27, 7.47, 5.60, 7.54]

# Create figure with 3 subplots (one per implementation)
fig, axes = plt.subplots(1, 3, figsize=(18, 6))

# Plot A1: Two-Copy
ax1 = axes[0]
ax1.plot(message_sizes, a1_cpb_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax1.plot(message_sizes, a1_cpb_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax1.plot(message_sizes, a1_cpb_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax1.plot(message_sizes, a1_cpb_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax1.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax1.set_ylabel('CPU Cycles per Byte', fontsize=12, fontweight='bold')
ax1.set_title('A1: Two-Copy (send/recv)', fontsize=13, fontweight='bold')
ax1.legend(loc='upper right')
ax1.grid(True, alpha=0.3, linestyle='--')
ax1.set_xscale('log', base=2)
ax1.set_xticks(message_sizes)
ax1.set_xticklabels([str(x) for x in message_sizes])

# Plot A2: One-Copy
ax2 = axes[1]
ax2.plot(message_sizes, a2_cpb_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax2.plot(message_sizes, a2_cpb_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax2.plot(message_sizes, a2_cpb_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax2.plot(message_sizes, a2_cpb_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax2.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax2.set_ylabel('CPU Cycles per Byte', fontsize=12, fontweight='bold')
ax2.set_title('A2: One-Copy (sendmsg + iovec)', fontsize=13, fontweight='bold')
ax2.legend(loc='upper right')
ax2.grid(True, alpha=0.3, linestyle='--')
ax2.set_xscale('log', base=2)
ax2.set_xticks(message_sizes)
ax2.set_xticklabels([str(x) for x in message_sizes])

# Plot A3: Zero-Copy
ax3 = axes[2]
ax3.plot(message_sizes, a3_cpb_t1, 'o-', linewidth=2, markersize=8, label='1 Thread')
ax3.plot(message_sizes, a3_cpb_t2, 's-', linewidth=2, markersize=8, label='2 Threads')
ax3.plot(message_sizes, a3_cpb_t4, '^-', linewidth=2, markersize=8, label='4 Threads')
ax3.plot(message_sizes, a3_cpb_t8, 'd-', linewidth=2, markersize=8, label='8 Threads')
ax3.set_xlabel('Message Size (bytes)', fontsize=12, fontweight='bold')
ax3.set_ylabel('CPU Cycles per Byte', fontsize=12, fontweight='bold')
ax3.set_title('A3: Zero-Copy (MSG_ZEROCOPY)', fontsize=13, fontweight='bold')
ax3.legend(loc='upper right')
ax3.grid(True, alpha=0.3, linestyle='--')
ax3.set_xscale('log', base=2)
ax3.set_xticks(message_sizes)
ax3.set_xticklabels([str(x) for x in message_sizes])

plt.suptitle('CPU Cycles per Byte vs Message Size\nRoll: MT25190 | System: Linux | Lower is better',
             fontsize=14, fontweight='bold', y=1.02)

plt.tight_layout()
plt.savefig('MT25190_CyclesPerByte.png', dpi=300, bbox_inches='tight')
print("Plot saved: MT25190_CyclesPerByte.png")
plt.show()
