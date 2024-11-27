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
2024-11-28T00:28:51+08:00
Running ./tt-alloc-opt
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.85, 0.81, 0.81
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-------------------------------------------------------------------------------------
Benchmark                                           Time             CPU   Iterations
-------------------------------------------------------------------------------------
FreeListOpt/WorstCase                           55520 ns        55320 ns        12692
FreeListOpt/TypicalCase                         30077 ns        29968 ns        23665
FreeListOpt/Small                                3408 ns         3397 ns       205894
FreeListOpt/GetAvailableAddresses                 416 ns          415 ns      1687898
FreeListOpt/Statistics                            377 ns          376 ns      1851977
FreeList[BestMatch]/WorstCase                 2316698 ns      2309410 ns          303
FreeList[BestMatch]/TypicalCase                903675 ns       900548 ns          772
FreeList[BestMatch]/Small                        8666 ns         8635 ns        81469
FreeList[BestMatch]/GetAvailableAddresses         687 ns          685 ns       971112
FreeList[BestMatch]/Statistics                   1098 ns         1095 ns       638956
FreeList[FirstMatch]/WorstCase                2347360 ns      2339262 ns          299
FreeList[FirstMatch]/TypicalCase               895072 ns       891667 ns          789
FreeList[FirstMatch]/Small                       8131 ns         8102 ns        86482
FreeList[FirstMatch]/GetAvailableAddresses        820 ns          818 ns       919654
FreeList[FirstMatch]/Statistics                  1102 ns         1098 ns       637792
```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.