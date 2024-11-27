// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tt_metal/impl/allocator/algorithms/free_list_opt.hpp"
#include <cstddef>
#include <cstdio>
#include <unordered_set>
#include <vector>
#include "tt_metal/impl/allocator/algorithms/allocator_algorithm.hpp"

namespace tt {

namespace tt_metal {

namespace allocator {

FreeListOpt::FreeListOpt(DeviceAddr max_size_bytes, DeviceAddr offset_bytes, DeviceAddr min_allocation_size, DeviceAddr alignment)
    : Algorithm(max_size_bytes, offset_bytes, min_allocation_size, alignment) {
    
    // Reduce reallocations by reserving memory for free list components
    constexpr size_t initial_block_count = 60;
    block_address_.reserve(initial_block_count);
    block_size_.reserve(initial_block_count);
    block_prev_block_.reserve(initial_block_count);
    block_next_block_.reserve(initial_block_count);
    block_is_allocated_.reserve(initial_block_count);
    free_meta_block_indices_.reserve(initial_block_count);
    free_blocks_segregated_by_size_.resize(size_seregation_count);
    for(auto& free_blocks : free_blocks_segregated_by_size_) {
        free_blocks.reserve(initial_block_count);
    }

    init();
}

void FreeListOpt::init()
{
    block_address_.clear();
    block_size_.clear();
    block_prev_block_.clear();
    block_next_block_.clear();
    block_is_allocated_.clear();
    free_meta_block_indices_.clear();
    allocated_block_table_.clear();
    for(auto& free_blocks : free_blocks_segregated_by_size_) {
        free_blocks.clear();
    }

    // Create a single block that spans the entire memory
    block_address_.push_back(0);
    block_size_.push_back(max_size_bytes_);
    block_prev_block_.push_back(-1);
    block_next_block_.push_back(-1);
    block_is_allocated_.push_back(false);
    free_blocks_segregated_by_size_[get_size_seregation_index(max_size_bytes_)].push_back(0);
}

std::optional<DeviceAddr> FreeListOpt::allocate(DeviceAddr size_bytes, bool bottom_up, DeviceAddr address_limit)
{
    DeviceAddr alloc_size = align(std::max(size_bytes, min_allocation_size_));

    // Find the best free block by looking at the segregated free blocks, it we can find a block in it's size class
    // we can be confident that it's the best block to allocate from. Else, look at the next size class. However the
    // blocks within a size class are not sorted by size, so we may not always find the best block.

    ssize_t target_block_index = -1;
    size_t size_seregation_index = get_size_seregation_index(alloc_size);
    TT_ASSERT(size_seregation_index < size_seregation_count, "Size seregation index out of bounds");
    std::vector<size_t>* segregated_list = nullptr;
    size_t segregated_item_index = 0;

    for(size_t i = size_seregation_index; i < size_seregation_count; i++) {
        auto& free_blocks = free_blocks_segregated_by_size_[i];
        for(size_t j = 0; j < free_blocks.size(); j++) {
            size_t block_index = free_blocks[j];
            if(block_size_[block_index] == alloc_size) {
                target_block_index = block_index;
                segregated_list = &free_blocks;
                segregated_item_index = j;
                break;
            }
            else if(block_size_[block_index] >= alloc_size && (target_block_index == -1 || block_size_[block_index] < block_size_[target_block_index])) {
                target_block_index = block_index;
                segregated_list = &free_blocks;
                segregated_item_index = j;
            }
        }
        if(target_block_index != -1) {
            break;
        }
    }

    if(target_block_index == -1) {
        return std::nullopt;
    }
    TT_ASSERT(segregated_list != nullptr, "Segegated list is null");
    TT_ASSERT(segregated_item_index < segregated_list->size(), "Segegated item index out of bounds");
    TT_ASSERT(block_is_allocated_[target_block_index] == false, "Block is already allocated");
    segregated_list->erase(segregated_list->begin() + segregated_item_index);


    // Allocate the block
    size_t offset = 0;
    if(!bottom_up) {
        offset = block_size_[target_block_index] - alloc_size;
    }
    size_t allocated_block_index = allocate_in_block(target_block_index, alloc_size, offset);
    return block_address_[allocated_block_index] + offset_bytes_;
    DeviceAddr start_address = block_address_[target_block_index];

    TT_FATAL(start_address + offset_bytes_ < address_limit || address_limit == 0, "Out of Memory: Cannot allocate at an address below {}. Allocation ends at {}", address_limit, start_address + offset_bytes_);
    TT_ASSERT(allocated_block_table_.find(start_address) == allocated_block_table_.end(), "Block already allocated");
    allocated_block_table_[start_address] = target_block_index;
    return block_address_[target_block_index] + offset_bytes_;
}

std::optional<DeviceAddr> FreeListOpt::allocate_at_address(DeviceAddr absolute_start_address, DeviceAddr size_bytes)
{
    // Nothing we can do but scan the free list
    size_t alloc_size = align(std::max(size_bytes, min_allocation_size_));
    ssize_t target_block_index = -1;
    for(size_t i = 0; i < block_address_.size(); i++) {
        if(block_address_[i] < absolute_start_address && block_address_[i] + block_size_[i] >= absolute_start_address + alloc_size) {
            target_block_index = i;
            break;
        }
    }

    if(target_block_index == -1 || block_is_allocated_[target_block_index]) {
        return std::nullopt;
    }

    // Find the relevant size seregation list
    size_t size_seregation_index = get_size_seregation_index(block_size_[target_block_index]);
    std::vector<size_t>& segregated_list = free_blocks_segregated_by_size_[size_seregation_index];
    TT_ASSERT(segregated_list.size() > 0, "Size seregation list is empty");
    for(size_t i = 0; i < segregated_list.size(); i++) {
        if(segregated_list[i] == target_block_index) {
            segregated_list.erase(segregated_list.begin() + i);
            break;
        }
    }

    size_t offset = absolute_start_address - block_address_[target_block_index];
    size_t alloc_block_index = allocate_in_block(target_block_index, alloc_size, offset);
    return block_address_[alloc_block_index] + offset_bytes_;
}

size_t FreeListOpt::allocate_in_block(size_t block_index, DeviceAddr alloc_size, size_t offset)
{
    if(block_size_[block_index] == alloc_size) {
        block_is_allocated_[block_index] = true;
        allocated_block_table_[block_address_[block_index]] = block_index;
        return block_index;
    }

    bool left_aligned = offset == 0;
    bool right_aligned = offset + alloc_size == block_size_[block_index];

    // Create free space if not left/right aligned

    if(!left_aligned) {
        size_t free_block_size = offset;
        DeviceAddr free_block_address = block_address_[block_index];
        ssize_t prev_block = block_prev_block_[block_index];
        ssize_t next_block = block_next_block_[block_index];
        block_size_[block_index] -= offset;
        block_address_[block_index] += offset;
        size_t new_block_index = alloc_meta_block(free_block_address, free_block_size, prev_block, block_index, false);
        if(prev_block != -1) {
            block_next_block_[prev_block] = new_block_index;
        }
        block_prev_block_[block_index] = new_block_index;

        size_t size_seregation_index = get_size_seregation_index(free_block_size);
        free_blocks_segregated_by_size_[size_seregation_index].push_back(new_block_index);
    }

    if(!right_aligned) {
        size_t free_block_size = block_size_[block_index] - alloc_size;
        DeviceAddr free_block_address = block_address_[block_index] + alloc_size;
        ssize_t prev_block = block_index;
        ssize_t next_block = block_next_block_[block_index];
        block_size_[block_index] -= free_block_size;
        size_t new_block_index = alloc_meta_block(free_block_address, free_block_size, prev_block, next_block, false);
        if(next_block != -1) {
            block_prev_block_[next_block] = new_block_index;
        }
        block_next_block_[block_index] = new_block_index;

        size_t size_seregation_index = get_size_seregation_index(free_block_size);
        free_blocks_segregated_by_size_[size_seregation_index].push_back(new_block_index);
    }
    block_is_allocated_[block_index] = true;
    allocated_block_table_[block_address_[block_index]] = block_index;

    return block_index;
}

void FreeListOpt::deallocate(DeviceAddr absolute_address)
{
    TT_ASSERT(allocated_block_table_.find(absolute_address - offset_bytes_) != allocated_block_table_.end(), "Block not allocated. Invalid address or double free");

    size_t block_index = allocated_block_table_[absolute_address - offset_bytes_];
    allocated_block_table_.erase(absolute_address - offset_bytes_);
    block_is_allocated_[block_index] = false;
    ssize_t prev_block = block_prev_block_[block_index];
    ssize_t next_block = block_next_block_[block_index];

    bool merged_prev = false;
    bool merged_next = false;

    // Merge with previous block if it's free
    if(prev_block != -1 && !block_is_allocated_[prev_block]) {
        block_size_[prev_block] += block_size_[block_index];
        block_next_block_[prev_block] = next_block;
        if(next_block != -1) {
            block_prev_block_[next_block] = prev_block;
        }
        free_meta_block(block_index);
        block_index = prev_block;
        merged_prev = true;

        // Look into the size segregated list to remove the block
        size_t size_seregation_index = get_size_seregation_index(block_size_[block_index]);
        std::vector<size_t>& segregated_list = free_blocks_segregated_by_size_[size_seregation_index];
        for(size_t i = 0; i < segregated_list.size(); i++) {
            if(segregated_list[i] == block_index) {
                segregated_list.erase(segregated_list.begin() + i);
                break;
            }
        }
    }

    // Merge with next block if it's free
    if(next_block != -1 && !block_is_allocated_[next_block]) {
        block_size_[block_index] += block_size_[next_block];
        block_next_block_[block_index] = block_next_block_[next_block];
        if(block_next_block_[next_block] != -1) {
            block_prev_block_[block_next_block_[next_block]] = block_index;
        }
        free_meta_block(next_block);
        merged_next = true;

        // Look into the size segregated list to remove the block
        size_t size_seregation_index = get_size_seregation_index(block_size_[block_index]);
        std::vector<size_t>& segregated_list = free_blocks_segregated_by_size_[size_seregation_index];
        for(size_t i = 0; i < segregated_list.size(); i++) {
            if(segregated_list[i] == next_block) {
                segregated_list.erase(segregated_list.begin() + i);
                break;
            }
        }
    }

    // Update the segregated list
    size_t block_size = block_size_[block_index];
    size_t size_seregation_index = get_size_seregation_index(block_size);
    free_blocks_segregated_by_size_[size_seregation_index].push_back(block_index);
}

std::vector<std::pair<DeviceAddr, DeviceAddr>> FreeListOpt::available_addresses(DeviceAddr size_bytes) const
{
    size_t alloc_size = align(std::max(size_bytes, min_allocation_size_));
    size_t size_seregation_index = get_size_seregation_index(alloc_size);
    std::vector<std::pair<DeviceAddr, DeviceAddr>> addresses;

    for(size_t i = size_seregation_index; i < size_seregation_count; i++) {
        for(size_t j = 0; j < free_blocks_segregated_by_size_[i].size(); j++) {
            size_t block_index = free_blocks_segregated_by_size_[i][j];
            if(block_size_[block_index] >= alloc_size) {
                addresses.push_back({block_address_[block_index], block_address_[block_index] + block_size_[block_index]});
            }
        }
    }
    return addresses;
}

size_t FreeListOpt::alloc_meta_block(DeviceAddr address, DeviceAddr size, ssize_t prev_block, ssize_t next_block, bool is_allocated)
{
    size_t idx;
    if(free_meta_block_indices_.empty()) {
        idx = block_address_.size();
        block_address_.push_back(address);
        block_size_.push_back(size);
        block_prev_block_.push_back(prev_block);
        block_next_block_.push_back(next_block);
        block_is_allocated_.push_back(is_allocated);
    } else {
        idx = free_meta_block_indices_.back();
        free_meta_block_indices_.pop_back();
        block_address_[idx] = address;
        block_size_[idx] = size;
        block_prev_block_[idx] = prev_block;
        block_next_block_[idx] = next_block;
        block_is_allocated_[idx] = is_allocated;
    }
    return idx;
}

void FreeListOpt::free_meta_block(size_t block_index)
{
    free_meta_block_indices_.push_back(block_index);
}

void FreeListOpt::clear()
{
    init();
}

Statistics FreeListOpt::get_statistics() const {
    // TODO: Cache the statistics
    size_t total_allocated_bytes = 0;
    size_t total_free_bytes = 0;
    size_t largest_free_block_bytes = 0;
    std::vector<uint32_t> largest_free_block_addrs;

    for(size_t i = 0; i < block_address_.size(); i++) {
        if(block_is_allocated_[i]) {
            total_allocated_bytes += block_size_[i];
        } else {
            total_free_bytes += block_size_[i];
            if(block_size_[i] >= largest_free_block_bytes) {
                largest_free_block_bytes = block_size_[i];
                largest_free_block_addrs.push_back(block_address_[i] + offset_bytes_);
            }
        }
    }

    if(total_allocated_bytes == 0) {
        total_free_bytes = max_size_bytes_;
        largest_free_block_bytes = max_size_bytes_;
    }

    return Statistics{
        .total_allocatable_size_bytes = max_size_bytes_,
        .total_allocated_bytes = total_allocated_bytes,
        .total_free_bytes = total_free_bytes,
        .largest_free_block_bytes = largest_free_block_bytes,
        .largest_free_block_addrs = std::move(largest_free_block_addrs),
    };
}

void FreeListOpt::dump_blocks(std::ostream &out) const
{
    out << "FreeListOpt allocator info:" << std::endl;
    out << "segregated free blocks by size:" << std::endl;
    for(size_t i = 0; i < free_blocks_segregated_by_size_.size(); i++) {
        if(i != free_blocks_segregated_by_size_.size() - 1)
            out << "  Size class " << i << ": (" << size_t(size_seregation_base * (size_t{1} << i)) << " - " << size_t(size_seregation_base * (size_t{1} << (i+1))) << ") blocks: ";
        else
            out << "  Size class " << i << ": (" << size_t(size_seregation_base * (size_t{1} << i)) << " - inf) blocks: ";
        for(size_t j = 0; j < free_blocks_segregated_by_size_[i].size(); j++) {
            out << free_blocks_segregated_by_size_[i][j] << " ";
        }

        out << std::endl;
    }

    out << "Free slots in block table: ";
    for(size_t i = 0; i < free_meta_block_indices_.size(); i++) {
        out << free_meta_block_indices_[i] << " ";
    }
    out << std::endl;

    out << "Block table:" << std::endl;
    auto leftpad = [](std::string str, size_t width) {
        if(str.size() >= width) {
            return str;
        }
        return std::string(width - str.size(), ' ') + str;
    };
    auto leftpad_num = [leftpad](auto num, size_t width) {
        // HACK: -1 for us means none
        if(num == -1) {
            return leftpad("none", width);
        }
        return leftpad(std::to_string(num), width);
    };
    const size_t pad = 10;
    std::array<std::string, 6> headers = {"Block", "Address", "Size", "PrevID", "NextID", "Allocated"};
    for(auto& header : headers) {
        out << leftpad(header, pad) << " ";
    }
    out << std::endl;
    std::unordered_set<size_t> free_meta_blocks(free_meta_block_indices_.begin(), free_meta_block_indices_.end());
    for(size_t i = 0; i < block_address_.size(); i++) {
        if(free_meta_blocks.find(i) != free_meta_blocks.end()) {
            continue;
        }
        out << leftpad_num(i, pad) << " " << leftpad_num(block_address_[i], pad) << " "
            << leftpad_num(block_size_[i], pad) << " " << leftpad_num(block_prev_block_[i], pad) << " " <<
            leftpad_num(block_next_block_[i], pad) << " " << leftpad(block_is_allocated_[i] ? "yes" : "no", pad) << std::endl;
    }
}

void FreeListOpt::shrink_size(DeviceAddr shrink_size, bool bottom_up)
{
    if(shrink_size == 0) {
        return;
    }
    TT_FATAL(bottom_up, "Shrinking from the top is currently not supported");
    TT_FATAL(
        shrink_size <= this->max_size_bytes_,
        "Shrink size {} must be smaller than max size {}",
        shrink_size,
        max_size_bytes_);
    
    // loop and scan the free list to find if the shrink cut into any allocated block
    size_t block_to_shrink = -1;
    for(size_t i = 0; i < block_address_.size(); i++) {
        if(block_is_allocated_[i]) {
            TT_FATAL(
                block_address_[i] >= shrink_size,
                "Shrink size {} cuts into allocated block at address {}",
                shrink_size,
                block_address_[i]);
        }
        else {
            if(block_address_[i] <= shrink_size && block_address_[i] + block_size_[i] >= shrink_size) {
                block_to_shrink = i;
                break;
            }
        }
    }

    TT_FATAL(block_to_shrink != -1, "Shrink size {} does not align with any block. This must be a bug", shrink_size);

    std::vector<size_t>* segregated_list = nullptr;
    size_t segregated_item_index = 0;

    // Find the relevant size seregation list
    size_t size_seregation_index = get_size_seregation_index(block_size_[block_to_shrink]);
    segregated_list = &free_blocks_segregated_by_size_[size_seregation_index];
    for(size_t i = 0; i < segregated_list->size(); i++) {
        if((*segregated_list)[i] == block_to_shrink) {
            segregated_item_index = i;
            break;
        }
    }
    segregated_list->erase(segregated_list->begin() + segregated_item_index);

    // Shrink the block
    block_size_[block_to_shrink] -= shrink_size;
    max_size_bytes_ -= shrink_size;
    shrink_size_ += shrink_size;
    if(block_size_[block_to_shrink] == 0) {
        block_prev_block_[block_next_block_[block_to_shrink]] = block_prev_block_[block_to_shrink];
        free_meta_block(block_to_shrink);
    } else {
        block_address_[block_to_shrink] += shrink_size;
        size_seregation_index = get_size_seregation_index(block_size_[block_to_shrink]);
        free_blocks_segregated_by_size_[size_seregation_index].push_back(block_to_shrink);
    }
}

void FreeListOpt::reset_size()
{
    if(shrink_size_ == 0) {
        return;
    }

    // Create a new block, mark it as allocated and deallocate the old block so coalescing can happen
    DeviceAddr lowest_address = ~(DeviceAddr)0;
    size_t lowest_block_index = -1;
    for(size_t i = 0; i < block_address_.size(); i++) {
        if(block_address_[i] < lowest_address) {
            lowest_address = block_address_[i];
            lowest_block_index = i;
        }
    }
    size_t new_block_index = alloc_meta_block(0, shrink_size_, -1, lowest_block_index, true);
    allocated_block_table_[0] = new_block_index;
    TT_ASSERT(block_prev_block_[lowest_block_index] == -1, "Lowest block should not have a previous block");
    block_prev_block_[lowest_block_index] = new_block_index;
    deallocate(0);

    max_size_bytes_ += shrink_size_;
    shrink_size_ = 0;
}

}
}
}