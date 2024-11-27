# tt-alloc-opt

Demo and benchmark program for a new allocator I'm writing for Tenstorrent's [tt-metal](https://github.com/tenstorrent/tt-metal) repository. As I can't stand the performance of the current one.

## How to build

You need a C++17 capable compiler and Google Benchmark installed

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
```

## Results

My allocator is orders of magnitude faster.

```
2024-11-28T01:18:04+08:00
Running ./tt-alloc-opt
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.97, 0.90, 0.85
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------
Benchmark                                           Time             CPU   Iterations
-------------------------------------------------------------------------------------
FreeListOpt/WorstCase                          112556 ns       112204 ns         6230
FreeListOpt/MixedAllocations                    55680 ns        55493 ns        12486
FreeListOpt/TypicalCase                         30600 ns        30509 ns        22802
FreeListOpt/Small                                3658 ns         3649 ns       190079
FreeListOpt/GetAvailableAddresses                 415 ns          413 ns      1735803
FreeListOpt/Statistics                            378 ns          377 ns      1861140
FreeList[BestMatch]/WorstCase                 8645952 ns      8602799 ns           81
FreeList[BestMatch]/MixedAllocations          2390974 ns      2381924 ns          306
FreeList[BestMatch]/TypicalCase                880462 ns       877334 ns          787
FreeList[BestMatch]/Small                        7669 ns         7646 ns        92969
FreeList[BestMatch]/GetAvailableAddresses         800 ns          797 ns       957576
FreeList[BestMatch]/Statistics                   1123 ns         1118 ns       639828
FreeList[FirstMatch]/WorstCase                7732526 ns      7702848 ns           90
FreeList[FirstMatch]/MixedAllocations         2399624 ns      2390824 ns          295
FreeList[FirstMatch]/TypicalCase               890122 ns       886873 ns          792
FreeList[FirstMatch]/Small                       7601 ns         7577 ns        89249
FreeList[FirstMatch]/GetAvailableAddresses        764 ns          761 ns       924639
FreeList[FirstMatch]/Statistics                  1107 ns         1104 ns       639846

```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.