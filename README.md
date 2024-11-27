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
2024-11-27T22:36:42+08:00
Running ./tt-alloc-opt
Run on (16 X 5132 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 1024 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 1.07, 0.94, 0.91
***WARNING*** CPU scaling is enabled, the benchmark real time measurements may be noisy and will incur extra overhead.
-----------------------------------------------------------------------------
Benchmark                                   Time             CPU   Iterations
-----------------------------------------------------------------------------
BM_FreeListOpt_WorstCase                58010 ns        57815 ns        11750
BM_FreeListOpt_TypicalCase              32047 ns        31917 ns        21094
BM_FreeList_WorstCase_BestMatch       2205831 ns      2198164 ns          318
BM_FreeList_TypicalCase_BestMatch      815403 ns       812663 ns          846
BM_FreeList_WorstCase_FirstMatch      2201758 ns      2195023 ns          318
BM_FreeList_TypicalCase_FirstMatch     813207 ns       810378 ns          841
```

## License

Fixtures and the original free list implementation is talken and modified from the tt-metal repository. Which is licensed under Apache 2.0.

The code I wrote is in 0 BSD. Because I am going to upstream it anyway.