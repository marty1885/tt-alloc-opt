#include <catch2/catch_test_macros.hpp>
#include "tt_metal/impl/allocator/algorithms/free_list_opt.hpp"

// UDL to convert integer literals to SI units
constexpr size_t operator"" _KiB(unsigned long long x) { return x * 1024; }
constexpr size_t operator"" _MiB(unsigned long long x) { return x * 1024 * 1024; }
constexpr size_t operator"" _GiB(unsigned long long x) { return x * 1024 * 1024 * 1024; }
TEST_CASE("Allocation") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(a.value() == 0);

    auto b = allocator.allocate(1_KiB);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 1_KiB);
}

TEST_CASE("Clear") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    auto b = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    allocator.clear();
    auto c = allocator.allocate(1_KiB);
    REQUIRE(c.has_value());
    REQUIRE(c.value() == 0);
}

TEST_CASE("Allocation and Deallocation") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    std::vector<std::optional<DeviceAddr>> allocations(10);

    for(size_t i = 0; i < allocations.size(); i++) {
        allocations[i] = allocator.allocate(1_KiB);
        REQUIRE(allocations[i].has_value());
    }

    SECTION("Deallocate in reverse order") {
        for(size_t i = allocations.size(); i > 0; i--) {
            allocator.deallocate(allocations[i - 1].value());
        }
    }
    SECTION("Deallocate in order") {
        for(size_t i = 0; i < allocations.size(); i++) {
            allocator.deallocate(allocations[i].value());
        }
    }
}

TEST_CASE("Allocate at address") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(a.value() == 0);

    auto b = allocator.allocate_at_address(1_KiB, 1_KiB);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 1_KiB);

    // Address is already allocated
    auto c = allocator.allocate_at_address(1_KiB, 1_KiB);
    REQUIRE(!c.has_value());

    auto d = allocator.allocate_at_address(2_KiB, 1_KiB);
    REQUIRE(d.has_value());
    REQUIRE(d.value() == 2_KiB);

    allocator.deallocate(a.value());
    auto e = allocator.allocate_at_address(0, 1_KiB);
    REQUIRE(e.has_value());
    REQUIRE(e.value() == 0);
}

TEST_CASE("Allocate at address interactions") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto wedge = allocator.allocate_at_address(32_KiB, 1_KiB);

    auto a = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(a.value() == 0);

    auto z = allocator.allocate(1_KiB, false);
    REQUIRE(z.has_value());
    REQUIRE(z.value() == 32_KiB - 1_KiB); // Counterintuitive, but because we use BestFit, it will find the smaller block at the beginning

    auto b = allocator.allocate(1_KiB);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 1_KiB);
}

TEST_CASE("Shrink and Reset") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    auto b = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    allocator.deallocate(a.value());

    allocator.shrink_size(1_KiB);
    auto c = allocator.allocate_at_address(0, 1_KiB);
    REQUIRE(!c.has_value());

    auto d = allocator.allocate_at_address(1_KiB, 1_KiB);
    REQUIRE(!d.has_value());

    allocator.reset_size();
    allocator.deallocate(b.value());

    auto e = allocator.allocate(2_KiB);
    REQUIRE(e.has_value());
}

TEST_CASE("Statistics") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    auto b = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    allocator.deallocate(a.value());

    auto stats = allocator.get_statistics();
    REQUIRE(stats.total_allocated_bytes == 1_KiB);
}

TEST_CASE("Allocate from top") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB, false);
    REQUIRE(a.has_value());
    REQUIRE(a.value() == 1_GiB - 1_KiB);

    auto b = allocator.allocate(1_KiB, false);
    REQUIRE(b.has_value());
    REQUIRE(b.value() == 1_GiB - 2_KiB);

    auto c = allocator.allocate(1_KiB);
    REQUIRE(c.has_value());
    REQUIRE(c.value() == 0);
}

TEST_CASE("Coalescing") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    auto b = allocator.allocate(1_KiB);
    auto c = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    allocator.deallocate(b.value());
    allocator.deallocate(a.value());
    
    auto d = allocator.allocate(2_KiB);
    REQUIRE(d.has_value());
    REQUIRE(d.value() == 0);
}

TEST_CASE("Coalescing after reset_shrink") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    auto a = allocator.allocate(1_KiB);
    auto b = allocator.allocate(1_KiB);
    auto c = allocator.allocate(1_KiB);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    allocator.deallocate(b.value());
    allocator.deallocate(a.value());
    
    allocator.shrink_size(1_KiB);
    auto d = allocator.allocate(2_KiB);
    allocator.reset_size();
    auto e = allocator.allocate(2_KiB);
    REQUIRE(e.has_value());
    REQUIRE(e.value() == 0);
}

TEST_CASE("Out of Memory") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    SECTION("Full alloc") {
        auto a = allocator.allocate(1_GiB);
        REQUIRE(a.has_value());
        auto b = allocator.allocate(1_KiB);
        REQUIRE(!b.has_value());
    }
    SECTION("Not enough space") {
        auto a = allocator.allocate(1_GiB - 1_KiB);
        REQUIRE(a.has_value());
        auto b = allocator.allocate(2_KiB);
        REQUIRE(!b.has_value());
    }
}

TEST_CASE("Avaliable Addresses") {
    auto allocator = tt::tt_metal::allocator::FreeListOpt(1_GiB, 0, 1_KiB, 1_KiB);
    SECTION("Simple") {
        auto a = allocator.allocate(1_KiB);
        auto aval = allocator.available_addresses(1_KiB);
        REQUIRE(aval.size() == 1);
        REQUIRE(aval[0].first == 1_KiB); // Start address
        REQUIRE(aval[0].second == 1_GiB); // End address
    }

    SECTION("Multiple") {
        auto a = allocator.allocate(1_KiB);
        auto b = allocator.allocate(1_KiB);
        auto c = allocator.allocate(1_KiB);
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 0);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == 1_KiB);
        REQUIRE(c.has_value());
        REQUIRE(c.value() == 2_KiB);
        allocator.deallocate(b.value());
        auto aval = allocator.available_addresses(1_KiB);
        REQUIRE(aval.size() == 2);
        REQUIRE(aval[0].first == 1_KiB); // Start address
        REQUIRE(aval[0].second == 2_KiB); // End address
        REQUIRE(aval[1].first == 3_KiB); // Start address
        REQUIRE(aval[1].second == 1_GiB); // End address
    }

    SECTION("With gaps") {
        auto a = allocator.allocate(1_KiB);
        auto b = allocator.allocate(1_KiB);
        auto c = allocator.allocate(1_KiB);
        REQUIRE(a.has_value());
        REQUIRE(a.value() == 0);
        REQUIRE(b.has_value());
        REQUIRE(b.value() == 1_KiB);
        REQUIRE(c.has_value());
        REQUIRE(c.value() == 2_KiB);
        allocator.deallocate(b.value());
        auto aval = allocator.available_addresses(10_KiB);
        REQUIRE(aval.size() == 1);
        REQUIRE(aval[0].first == 3_KiB); // Start address
        REQUIRE(aval[0].second == 1_GiB); // End address
    }
}