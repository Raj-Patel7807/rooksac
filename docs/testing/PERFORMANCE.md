# Performance principles

Performance matters because chess search expands rapidly, but optimization is a measured engineering activity.

## Likely concerns

- **Cache locality:** Compact, contiguous hot data can reduce memory stalls.
- **Allocation:** Repeated allocation in a search tree can dominate runtime; establish ownership first.
- **Branching factor:** Alpha-beta, ordering, and quiescence aim to reduce unproductive nodes.
- **Board operations:** Efficient bit operations can help attack and occupancy queries.
- **Move ordering:** Earlier cutoffs reduce the tree more than many micro-optimizations.

## Method

Profile representative workloads before changing code, identify the bottleneck, make an isolated change, preserve correctness tests, and compare under the [benchmarking](BENCHMARKING.md) protocol. Do not promise NPS, depth, memory, or playing-strength targets without evidence.
