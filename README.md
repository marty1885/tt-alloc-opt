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

My allocator is up to 40x faster then the current one in tt-metal.

```
2024-11-27T23:31:07+08:00
Running ./tt-alloc-opt
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.32, 1.11, 1.00
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------
Benchmark                                           Time             CPU   Iterations
-------------------------------------------------------------------------------------
FreeListOpt/WorstCase                           57382 ns        57208 ns        12152
FreeListOpt/TypicalCase                         31213 ns        31113 ns        22447
FreeListOpt/Small                                3602 ns         3589 ns       193188
FreeListOpt/GetAvailableAddresses                 432 ns          430 ns      1666265
FreeListOpt/Statistics                            368 ns          367 ns      1913711
FreeList[BestMatch]/WorstCase                 2241591 ns      2234036 ns          309
FreeList[BestMatch]/TypicalCase                827512 ns       824720 ns          830
FreeList[BestMatch]/Small                        6058 ns         6037 ns       115747
FreeList[BestMatch]/GetAvailableAddresses         811 ns          808 ns       856690
FreeList[BestMatch]/Statistics                   1078 ns         1074 ns       647449
FreeList[FirstMatch]/WorstCase                2262505 ns      2254319 ns          302
FreeList[FirstMatch]/TypicalCase               832531 ns       829469 ns          825
FreeList[FirstMatch]/Small                       5594 ns         5575 ns       124603
FreeList[FirstMatch]/GetAvailableAddresses        790 ns          788 ns       965669
FreeList[FirstMatch]/Statistics                  1078 ns         1075 ns       641301
```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.