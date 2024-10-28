# Cache Simulation

## Overview:
This project involves simulating a cache system with different configurations for L1 and L2 caches, including varying block sizes, cache sizes, and associativities. The objective is to evaluate the performance of cache systems under various configurations and analyze the impact on total access time.

## Key Components:

### 1. Design Decisions:
- **Inclusivity**: The system ensures that if a block is present in the L1 cache, it must also be present in the L2 cache. When a block is evicted from L1, it is also removed from L2, maintaining inclusivity.
- **Read Command**: If an L1 cache read misses but the block is present in L2, the dirty block from L1 is first written back to L2 before the new block is fetched. A similar approach is taken for L2 read misses.
- **Write Command**: The simulation enforces write-back and write-allocate policies. In the case of L1 write misses, the data is written only to L1, with writes to L2 occurring only during write-backs from L1.
- **Least Recently Used (LRU) Policy**: LRU is managed using a 2D vector where each block's usage is tracked. Upon each access, values are updated to reflect the most recently used block, reducing read and write misses.

### 2. Performance Graphs:
- **Total Access Time vs Block Size**: Demonstrates how total access time changes as the block size increases from 8 to 128 bytes. Initially, access time decreases due to improved spatial locality, but eventually increases as the number of sets decreases.
- **Total Access Time vs L1 Size**: Shows how increasing L1 cache size reduces total access time, with larger caches resulting in fewer capacity misses and faster access times.
- **Total Access Time vs L1 Associativity**: Illustrates the impact of increasing L1 cache associativity on access time. Associativity initially reduces access time but shows diminishing returns as it increases further.
- **Total Access Time vs L2 Size**: Evaluates the effect of varying L2 cache size on access time. The results indicate minimal change in total time as capacity misses are already low.
- **Total Access Time vs L2 Associativity**: Displays how increasing L2 associativity affects total access time, showing improvements with higher associativity.

### 3. Observations:
- **Cache Size**: Increasing the L1 cache size significantly reduces total access time by minimizing capacity misses. However, increasing L2 cache size has minimal impact on performance due to the low number of capacity misses.
- **Block Size**: Total access time decreases initially as the block size increases, benefiting from spatial locality, but beyond a certain point, performance degrades due to fewer sets.
- **Associativity**: Higher cache associativity improves performance initially, but excessive associativity shows diminishing returns, and total access time may even increase in certain cases.
