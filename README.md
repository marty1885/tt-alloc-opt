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
2024-11-28T11:09:30+08:00
Running ./tt-alloc-opt
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.02, 0.90, 0.84
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------
Benchmark                                           Time             CPU   Iterations
-------------------------------------------------------------------------------------
FreeListOpt/WorstCase                          156211 ns       155689 ns         4428
FreeListOpt/MixedAllocations                    59938 ns        59760 ns        11755
FreeListOpt/TypicalCase                         32595 ns        32492 ns        21033
FreeListOpt/Small                                3676 ns         3663 ns       179884
FreeListOpt/GetAvailableAddresses                 409 ns          407 ns      1726829
FreeListOpt/Statistics                            379 ns          378 ns      1848454
FreeListOpt/ShrinkReset                          11.7 ns         11.6 ns     60177702
FreeList[BestMatch]/WorstCase                 8430132 ns      8396434 ns           81
FreeList[BestMatch]/MixedAllocations          2219312 ns      2211079 ns          314
FreeList[BestMatch]/TypicalCase                863138 ns       860375 ns          802
FreeList[BestMatch]/Small                        7968 ns         7944 ns        85656
FreeList[BestMatch]/GetAvailableAddresses         893 ns          890 ns       779186
FreeList[BestMatch]/Statistics                   1107 ns         1103 ns       643461
FreeList[BestMatch]/ShrinkReset                  3.44 ns         3.43 ns    204268507
FreeList[FirstMatch]/WorstCase                7409143 ns      7380524 ns           95
FreeList[FirstMatch]/MixedAllocations         2283444 ns      2274426 ns          296
FreeList[FirstMatch]/TypicalCase               864990 ns       861908 ns          809
FreeList[FirstMatch]/Small                       7726 ns         7705 ns        91195
FreeList[FirstMatch]/GetAvailableAddresses        886 ns          882 ns       794174
FreeList[FirstMatch]/Statistics                  1092 ns         1088 ns       645254
FreeList[FirstMatch]/ShrinkReset                 3.47 ns         3.45 ns    202517500
```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.