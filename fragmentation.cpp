#include "tt_metal/impl/allocator/algorithms/allocator_algorithm.hpp"
#include "tt_metal/impl/allocator/algorithms/free_list_opt.hpp"
#include "tt_metal/impl/allocator/algorithms/free_list.hpp"

#include <random>

size_t test_allocator(tt::tt_metal::allocator::Algorithm& allocator, size_t alloc_size, size_t seed = 42)
{
    std::mt19937 gen(seed);
    std::uniform_int_distribution<size_t> alloc_size_dist(1, alloc_size);
    std::uniform_real_distribution<double> deallocate_dist(0.0, 1.0);
    std::vector<std::optional<DeviceAddr>> allocations;

    size_t i = 0;
    for (;; i++) {
        size_t size = alloc_size_dist(gen);
        auto addr = allocator.allocate(size);
        if(!addr.has_value()) {
            break;
        }
        allocations.push_back(addr);
        if(deallocate_dist(gen) < 0.7) {
            std::uniform_int_distribution<size_t> index_dist(0, allocations.size() - 1);
            size_t index = index_dist(gen);
            if(allocations[index].has_value()) {
                allocator.deallocate(*allocations[index]);
                allocations[index] = std::nullopt;
            }
        }
    }
    return i;
}


int main()
{
    size_t mem_size = 1.5 * 1024 * 1024; // 1.5 MB (Wormhole L1 size)
    size_t alloc_size = 16 * 1024; // 16 KB

    tt::tt_metal::allocator::FreeListOpt opt(mem_size, 0, 16, 16);
    tt::tt_metal::allocator::FreeList first(mem_size, 0, 16, 16, tt::tt_metal::allocator::FreeList::SearchPolicy::FIRST);
    tt::tt_metal::allocator::FreeList best(mem_size, 0, 16, 16, tt::tt_metal::allocator::FreeList::SearchPolicy::BEST);
    
    std::cout << "Benchmarking fragmentation... (number of allocation attempts until full)" << std::endl;
    std::cout << "FreeListOpt: " << test_allocator(opt, alloc_size) << std::endl;
    std::cout << "FreeList (First): " << test_allocator(first, alloc_size) << std::endl;
    std::cout << "FreeList (Best): " << test_allocator(best, alloc_size) << std::endl;
}