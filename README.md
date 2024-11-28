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
2024-11-29T00:15:34+08:00
Running ./tt-alloc-opt-bench
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.89, 0.82, 0.81
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------
Benchmark                                           Time             CPU   Iterations
-------------------------------------------------------------------------------------
FreeListOpt/WorstCase                          118577 ns       118086 ns         5863
FreeListOpt/MixedAllocations                    59143 ns        58927 ns        11783
FreeListOpt/TypicalCase                         33745 ns        33621 ns        21207
FreeListOpt/Small                                3529 ns         3519 ns       197782
FreeListOpt/GetAvailableAddresses                 408 ns          406 ns      1764844
FreeListOpt/Statistics                            383 ns          381 ns      1839916
FreeListOpt/ShrinkReset                          11.9 ns         11.8 ns     60195283
FreeList[BestMatch]/WorstCase                 8801668 ns      8770764 ns           80
FreeList[BestMatch]/MixedAllocations          2258415 ns      2250632 ns          305
FreeList[BestMatch]/TypicalCase                878087 ns       875110 ns          806
FreeList[BestMatch]/Small                        7699 ns         7679 ns        92646
FreeList[BestMatch]/GetAvailableAddresses         756 ns          753 ns       981334
FreeList[BestMatch]/Statistics                   1093 ns         1089 ns       644297
FreeList[BestMatch]/ShrinkReset                  3.51 ns         3.49 ns    203587009
FreeList[FirstMatch]/WorstCase                7577280 ns      7550615 ns           92
FreeList[FirstMatch]/MixedAllocations         2250868 ns      2243382 ns          309
FreeList[FirstMatch]/TypicalCase               866912 ns       863442 ns          817
FreeList[FirstMatch]/Small                       7348 ns         7321 ns        96339
FreeList[FirstMatch]/GetAvailableAddresses        743 ns          741 ns       966950
FreeList[FirstMatch]/Statistics                  1097 ns         1093 ns       636790
FreeList[FirstMatch]/ShrinkReset                 3.46 ns         3.45 ns    202568406
```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.