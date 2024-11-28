#include <benchmark/benchmark.h>
#include <optional>

#include "tt_metal/impl/allocator/algorithms/allocator_algorithm.hpp"
#include "tt_metal/impl/allocator/algorithms/free_list_opt.hpp"
#include "tt_metal/impl/allocator/algorithms/free_list.hpp"
namespace bm = benchmark;

// UDL to convert integer literals to SI units
constexpr size_t operator"" _KiB(unsigned long long x) { return x * 1024; }
constexpr size_t operator"" _MiB(unsigned long long x) { return x * 1024 * 1024; }
constexpr size_t operator"" _GiB(unsigned long long x) { return x * 1024 * 1024 * 1024; }

void bench_typical(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    std::vector<size_t> allocation_sizes = {64_KiB, 64_KiB, 120_KiB, 60_MiB, 256_KiB, 12_KiB, 16_MiB, 1_KiB};
    std::vector<size_t> temp_allocations = {16_KiB, 16_KiB, 16_KiB, 16_MiB, 32_KiB, 1_KiB, 1_MiB, 3_KiB};
    std::vector<std::optional<DeviceAddr>> allocations(allocation_sizes.size());
    std::vector<std::optional<DeviceAddr>> temp_allocs(temp_allocations.size());
    assert(allocation_sizes.size() == temp_allocations.size());
    size_t n_runs = 100;
    for (auto _ : state) {
        state.PauseTiming();
        allocator.clear();
        state.ResumeTiming();

        for(size_t i = 0; i < n_runs; i++) {
            for(size_t j = 0; j < allocation_sizes.size(); j++) {
                allocations[j] = allocator.allocate(allocation_sizes[j]);
                temp_allocs[j] = allocator.allocate(temp_allocations[j]);
            }

            for(size_t j = 0; j < allocation_sizes.size(); j++) {
                allocator.deallocate(temp_allocs[j].value());
            }
        }
    }
}

void bench_mixed(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    std::vector<size_t> allocation_sizes = {64_KiB, 64_KiB, 120_KiB, 60_MiB, 256_KiB, 12_KiB, 16_MiB, 1_KiB};
    std::vector<size_t> temp_allocations = {16_KiB, 16_KiB, 16_KiB, 16_MiB, 32_KiB, 1_KiB, 1_MiB, 3_KiB};
    std::vector<std::optional<DeviceAddr>> allocations(allocation_sizes.size());
    std::vector<std::optional<DeviceAddr>> temp_allocs(temp_allocations.size());
    assert(allocation_sizes.size() == temp_allocations.size());
    size_t n_runs = 150;
    for (auto _ : state) {
        state.PauseTiming();
        allocator.clear();
        state.ResumeTiming();

        for(size_t i = 0; i < n_runs; i++) {
            for(size_t j = 0; j < allocation_sizes.size(); j++) {
                allocations[j] = allocator.allocate(allocation_sizes[j]);
            }

            for(size_t j = 0; j < temp_allocations.size(); j++) {
                temp_allocs[j] = allocator.allocate(temp_allocations[j]);
            }

            for(size_t j = 0; j < allocation_sizes.size(); j++) {
                allocator.deallocate(temp_allocs[j].value());
            }
        }
    }
}

void bench_worst(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    size_t n_runs = 1000;
    for (auto _ : state) {
        state.PauseTiming();
        allocator.clear();
        state.ResumeTiming();

        for(size_t i = 0; i < n_runs; i++) {
            auto a = allocator.allocate(1_KiB);
            auto b = allocator.allocate(2_KiB);
            auto c = allocator.allocate(3_KiB);
            auto d = allocator.allocate(4_KiB);
            allocator.deallocate(a.value());
            allocator.deallocate(c.value());
        }
    }
}

void bench_small(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    size_t n_runs = 20;
    for (auto _ : state) {
        state.PauseTiming();
        allocator.clear();
        state.ResumeTiming();

        for(size_t i = 0; i < n_runs; i++) {
            auto a = allocator.allocate(1_KiB);
            allocator.deallocate(a.value());
            a = allocator.allocate(1_KiB);
            auto b = allocator.allocate(2_KiB);
            auto c = allocator.allocate(3_KiB);
            auto d = allocator.allocate_at_address(25_MiB, 2_KiB);
            auto e = allocator.allocate(4_KiB);
            allocator.deallocate(a.value());
            allocator.deallocate(b.value());
            allocator.deallocate(e.value());
            allocator.deallocate(c.value());
        }
    }
}

void bench_get_available_addresses(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    std::vector<std::optional<DeviceAddr>> allocations(450);
    for(size_t i = 0; i < allocations.size(); i++) {
        allocations[i] = allocator.allocate(1_KiB);
    }
    for(size_t i = 0; i < allocations.size(); i+=2) {
        allocator.deallocate(allocations[i].value());
    }
    for (auto _ : state) {
        bm::DoNotOptimize(allocator.available_addresses(1_KiB));
    }
}

void bench_statistics(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    std::vector<std::optional<DeviceAddr>> allocations(450);
    for(size_t i = 0; i < allocations.size(); i++) {
        allocations[i] = allocator.allocate(1_KiB);
    }
    for(size_t i = 0; i < allocations.size(); i+=2) {
        allocator.deallocate(allocations[i].value());
    }
    for (auto _ : state) {
        bm::DoNotOptimize(allocator.get_statistics());
    }
}

void bench_shrink_reset(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    // auto a = allocator.allocate(20_KiB, false);
    // auto b = allocator.allocate(20_KiB, false);
    // allocator.deallocate(a.value());
    for (auto _ : state) {
        allocator.shrink_size(1_KiB);
        allocator.reset_size();
    }
}

template <typename Allocator, typename BenchFunc, typename ... Args>
void RegisterBenchmark(const std::string& name, BenchFunc func, Args&& ... args) {
    auto benchmark_func = [=](bm::State& state) {
        Allocator allocator(args...);
        func(allocator, state);
    };
    bm::RegisterBenchmark(name.c_str(), benchmark_func);
}

template <typename Allocator, typename ... Args>
void RegisterBenchmarksForAllocator(const std::string& allocator_name, Args&& ... args) {
    size_t memory_size = 12_GiB;
    size_t alignment = 0;
    size_t min_alloc_size = 64;
    size_t max_alloc_size = 64;

    std::vector<std::pair<std::string, std::function<void(tt::tt_metal::allocator::Algorithm&, bm::State&)>>> benchmarks = {
        {"WorstCase", bench_worst},
        {"MixedAllocations", bench_mixed},
        {"TypicalCase", bench_typical},
        {"Small", bench_small},
        {"GetAvailableAddresses", bench_get_available_addresses},
        {"Statistics", bench_statistics},
        {"ShrinkReset", bench_shrink_reset}
    };

    for(auto& [name, func] : benchmarks) {
        RegisterBenchmark<Allocator>(allocator_name + "/" + name, func, memory_size, alignment, min_alloc_size, max_alloc_size, args...);
    }
}

void RegisterAllBenchmarks() {
    RegisterBenchmarksForAllocator<tt::tt_metal::allocator::FreeListOpt>("FreeListOpt");
    RegisterBenchmarksForAllocator<tt::tt_metal::allocator::FreeList>("FreeList[BestMatch]", tt::tt_metal::allocator::FreeList::SearchPolicy::BEST);
    RegisterBenchmarksForAllocator<tt::tt_metal::allocator::FreeList>("FreeList[FirstMatch]", tt::tt_metal::allocator::FreeList::SearchPolicy::FIRST);
}

int main(int argc, char** argv) {
    bm::Initialize(&argc, argv);
    RegisterAllBenchmarks();
    bm::RunSpecifiedBenchmarks();

    // auto allocator = tt::tt_metal::allocator::FreeListOpt(12_GiB, 0, 64, 64);
    // allocator.shrink_size(1_KiB);
    // allocator.reset_size();
    // allocator.shrink_size(1_KiB);
    // allocator.reset_size();
    // allocator.dump_blocks(std::cout);
}