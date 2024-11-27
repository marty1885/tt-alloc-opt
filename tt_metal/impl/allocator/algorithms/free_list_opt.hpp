// Copyright Martin Change <marty188586@gmail.com>
// 
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
// 
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>

#include "tt_metal/impl/allocator/algorithms/allocator_algorithm.hpp"

namespace tt {
namespace tt_metal {
namespace allocator {
// Essentially the same free list algorithm as FreeList, but with (IMO absurdly) optimized implementations. Including
// - SoA instead of linked list for the free list
// - Size segregated for faster best-fit block search
// - Hash table to store allocated blocks for faster block lookup during deallocation
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

    // Metadata block indices that is not currently used (to reuse blocks instead of always allocating new ones)
    std::vector<size_t> free_meta_block_indices_;

    // Caches so most operations don't need to scan the entire free list
    inline static constexpr size_t n_alloc_table_buckets = 512;
    inline static constexpr size_t n_alloc_table_bucket_size = 10;
    std::vector<std::vector<std::pair<DeviceAddr, size_t>>> allocated_block_table_;
    std::vector<std::vector<size_t>> free_blocks_segregated_by_size_;

    // internal functions
    // Given a block index, mark a chunk (from block start + offset to block start + offset + alloc_size) as allocated
    // Unused space is split into a new free block and retuened to the free list and the segregated list
    // NOTE: This function DOES NOT remove block_index from the segregated list. Caller should do that
    size_t allocate_in_block(size_t block_index, DeviceAddr alloc_size, size_t offset);

    inline static constexpr size_t size_segregated_base = 1024;
    inline static constexpr size_t size_segregated_count = 12;
    inline static size_t get_size_segregated_index(DeviceAddr size_bytes) {
        // std::log2 is SLOW, so we use a simple log2 implementation for integers
        size_t lg = 0;
        size_t n = size_bytes / size_segregated_base;
        while(n >>= 1) {
            lg++;
        }
        return std::min(size_segregated_count - 1, lg);
    }
    // Put the block at block_index into the size segregated list at the appropriate index (data taken from 
    // the SoA vectors)
    void insert_block_to_segregated_list(size_t block_index);

    // Allocate a new block and return the index to the block
    size_t alloc_meta_block(DeviceAddr address, DeviceAddr size, ssize_t prev_block, ssize_t next_block, bool is_allocated);
    // Free the block at block_index and mark it as free
    void free_meta_block(size_t block_index);

    // Operations on the allocated block table
    size_t hash_device_address(DeviceAddr address) const;
    void insert_block_to_alloc_table(DeviceAddr address, size_t block_index);
    bool is_address_in_alloc_table(DeviceAddr address) const;
    size_t get_and_remove_from_alloc_table(DeviceAddr address);
};

}  // namespace allocator
}  // namespace tt_metal
}  // namespace tt
