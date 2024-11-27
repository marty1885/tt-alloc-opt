// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <map>
#include <optional>

#include "tt_metal/impl/allocator/algorithms/allocator_algorithm.hpp"

namespace tt {
namespace tt_metal {
namespace allocator {
// Essentially the same free list algorithm as FreeList, but with (much) optimized implementations. Including
// - SoA instead of linked list for the free list
// - Size segregated for faster best-fit block search
// - RB Tree to look up allocated blocks by address O(log n) instead of O(n) walk
// - Keeps metadata locality to avoid cache misses
class FreeListOpt : public Algorithm {
    public:
    FreeListOpt(DeviceAddr max_size_bytes, DeviceAddr offset_bytes, DeviceAddr min_allocation_size, DeviceAddr alignment);
    void init() override;

    std::vector<std::pair<DeviceAddr, DeviceAddr>> available_addresses(DeviceAddr size_bytes) const override;

    std::optional<DeviceAddr> allocate(DeviceAddr size_bytes, bool bottom_up=true, DeviceAddr address_limit=0) override;

    std::optional<DeviceAddr> allocate_at_address(DeviceAddr absolute_start_address, DeviceAddr size_bytes) override;

    void deallocate(DeviceAddr absolute_address) override;

    void clear() override;

    Statistics get_statistics() const override;

    void dump_blocks(std::ostream &out) const override;

    void shrink_size(DeviceAddr shrink_size, bool bottom_up=true) override;

    void reset_size() override;

    private:

    // SoA free list components
    std::vector<DeviceAddr> block_address_;
    std::vector<DeviceAddr> block_size_;
    std::vector<ssize_t> block_prev_block_;
    std::vector<ssize_t> block_next_block_;
    std::vector<uint8_t> block_is_allocated_; // not using bool to avoid compacting

    // Metadata block indices (used so when a block is meged/removed, so the SoA don't always grow)
    std::vector<size_t> free_meta_block_indices_;

    // cache to enable faster block search
    std::unordered_map<DeviceAddr, size_t> allocated_block_table_;
    std::vector<std::vector<size_t>> free_blocks_segregated_by_size_;

    size_t allocate_in_block(size_t block_index, DeviceAddr alloc_size, size_t offset);

    inline static const size_t size_segregated_base = 1024;
    inline static const size_t size_segregated_count = 12;
    static size_t get_size_segregated_index(DeviceAddr size_bytes) {
        size_t lg = 0;
        size_t n = size_bytes / size_segregated_base;
        while(n >>= 1) {
            lg++;
        }
        return std::min(size_segregated_count - 1, lg);
    }

    size_t alloc_meta_block(DeviceAddr address, DeviceAddr size, ssize_t prev_block, ssize_t next_block, bool is_allocated);
    void free_meta_block(size_t block_index);

    void insert_block_to_segregated_list(size_t block_index);
};

}  // namespace allocator
}  // namespace tt_metal
}  // namespace tt
