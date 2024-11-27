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
        allocator.clear();

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

void bench_worst(tt::tt_metal::allocator::Algorithm& allocator, bm::State& state) {
    std::vector<size_t> allocation_sizes = {64_KiB, 64_KiB, 120_KiB, 60_MiB, 256_KiB, 12_KiB, 16_MiB, 1_KiB};
    std::vector<size_t> temp_allocations = {16_KiB, 16_KiB, 16_KiB, 16_MiB, 32_KiB, 1_KiB, 1_MiB, 3_KiB};
    std::vector<std::optional<DeviceAddr>> allocations(allocation_sizes.size());
    std::vector<std::optional<DeviceAddr>> temp_allocs(temp_allocations.size());
    assert(allocation_sizes.size() == temp_allocations.size());
    size_t n_runs = 150;
    for (auto _ : state) {
        allocator.clear();

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

static void BM_FreeListOpt_WorstCase(bm::State& state) {
    // Emulate a Wormhole device with 12 GiB of memory
    tt::tt_metal::allocator::FreeListOpt allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64);
    bench_worst(allocator, state);
}
static void BM_FreeListOpt_TypicalCase(bm::State& state) {
    // Emulate a Wormhole device with 12 GiB of memory
    tt::tt_metal::allocator::FreeListOpt allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64);
    bench_typical(allocator, state);
}

static void BM_FreeList_WorstCase_BestMatch(bm::State& state) {
    tt::tt_metal::allocator::FreeList allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64, tt::tt_metal::allocator::FreeList::SearchPolicy::BEST);
    bench_worst(allocator, state);
}
static void BM_FreeList_TypicalCase_BestMatch(bm::State& state) {
    tt::tt_metal::allocator::FreeList allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64, tt::tt_metal::allocator::FreeList::SearchPolicy::BEST);
    bench_typical(allocator, state);
}

static void BM_FreeList_WorstCase_FirstMatch(bm::State& state) {
    tt::tt_metal::allocator::FreeList allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64, tt::tt_metal::allocator::FreeList::SearchPolicy::FIRST);
    bench_worst(allocator, state);
}
static void BM_FreeList_TypicalCase_FirstMatch(bm::State& state) {
    tt::tt_metal::allocator::FreeList allocator(1024UL * 1024 * 1024 * 12, 0, 64, 64, tt::tt_metal::allocator::FreeList::SearchPolicy::FIRST);
    bench_typical(allocator, state);
}



BENCHMARK(BM_FreeListOpt_WorstCase);
BENCHMARK(BM_FreeListOpt_TypicalCase);
BENCHMARK(BM_FreeList_WorstCase_BestMatch);
BENCHMARK(BM_FreeList_TypicalCase_BestMatch);
BENCHMARK(BM_FreeList_WorstCase_FirstMatch);
BENCHMARK(BM_FreeList_TypicalCase_FirstMatch);

int main(int argc, char** argv) {
    bm::Initialize(&argc, argv);
    bm::RunSpecifiedBenchmarks();
}
