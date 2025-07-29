#include <cassert>
#include <functional>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>

#include "lsb.hpp"
#include "cdb.hpp"
#include "decoder.hpp"
#include "rob_types.hpp"
#include "reset.hpp"
#include "third_party/logger.hpp"
#include "utility/buffered.hpp"
#include "utility/clock.hpp"

using namespace norb::riscv;

// Test helper class to create mock ROB pointers
class MockROBPointer {
public:
    static rob_pointer_t create_mock_pointer(size_t index = 0) {
        auto pointer = rob_pointer_t::make_dummy(index);
        return pointer;
    }
};

// Test utilities for LSB
class LSBTestUtils {
public:
    static ResolvedInstructionEntry create_load_entry(InsType type, uint32_t base_addr, int32_t offset = 0, size_t rob_index = 0) {
        ResolvedInstructionEntry entry;
        entry.type = type;
        entry.vj = base_addr;  // base address
        entry.vk = 0;          // not used for loads
        entry.imm = offset;    // offset
        entry.rob_pointer = MockROBPointer::create_mock_pointer(rob_index);
        entry.pc = 0x1000;
        entry.had_jumped = false;
        return entry;
    }
    
    static ResolvedInstructionEntry create_store_entry(InsType type, uint32_t base_addr, uint32_t value, int32_t offset = 0, size_t rob_index = 0) {
        ResolvedInstructionEntry entry;
        entry.type = type;
        entry.vj = base_addr;  // base address
        entry.vk = value;      // value to store
        entry.imm = offset;    // offset
        entry.rob_pointer = MockROBPointer::create_mock_pointer(rob_index);
        entry.pc = 0x1000;
        entry.had_jumped = false;
        return entry;
    }
    
    static ResolverEntry create_resolver_entry(InsType type, uint32_t vj, uint32_t vk, int32_t imm, 
                                             bool j_ready = true, bool k_ready = true, size_t rob_index = 0) {
        ResolverEntry entry;
        entry.type = type;
        entry.status = (j_ready && k_ready) ? ResolverEntryStatus::READY : ResolverEntryStatus::PENDING;
        entry.rob_pointer = MockROBPointer::create_mock_pointer(rob_index);
        entry.k_is_ready = k_ready;
        entry.j_is_ready = j_ready;
        entry.vk = vk;
        entry.vj = vj;
        entry.imm = imm;
        entry.pc = 0x1000;
        entry.had_jumped = false;
        return entry;
    }
    
    // Create a simple test memory file
    static void create_test_memory_file(const std::string& filename) {
        std::ofstream file(filename);
        file << "@0\n";
        file << "01 02 03 04 05 06 07 08\n";  // Some test data
        file << "09 0A 0B 0C 0D 0E 0F 10\n";
        file << "@1000\n";
        file << "AA BB CC DD EE FF 11 22\n";  // Test data at address 0x1000
        file.close();
    }
};

// Auxiliary function to wrap each cycle operation
void in_cycle(int cycle_number, std::function<void()> cycle_operations) {
    std::cout << "Cycle " << cycle_number << std::endl;
    cycle_operations();
    norb::buffered_flush();
    norb::Clock::instance().tick();
}

int main() {
    Logger::get().setLevel(LogLevel::DEBUG);
    norb::Clock::instance().reset();

    // Create test memory file
    const std::string test_mem_file = "test_memory.data";
    LSBTestUtils::create_test_memory_file(test_mem_file);

    LoadStoreBuffer lsb;
    auto shared_cdb = std::make_shared<CommonDataBus>();
    norb::ChannelWriter<ResolverEntry> chw;
    norb::TemporaryBus<rob_pointer_t> commit_bus;

    // Setup LSB
    lsb.load_memory(test_mem_file);
    lsb.resolver.load_cdb(shared_cdb);
    lsb.resolver.bind_inbound_to(chw);
    lsb.set_commit_bus(commit_bus);

    int failures = 0;

    std::cout << "=== LSB Unit Tests ===" << std::endl;

    // Test 1: Basic memory reading from committed memory
    std::cout << "\n--- Test 1: Basic Memory Reading ---" << std::endl;
    
    in_cycle(1, [&]() {
        // Issue a load byte instruction to read from address 0
        auto load_entry = LSBTestUtils::create_resolver_entry(LB, 0, 0, 0, true, true, 1);
        chw.write(load_entry);
        lsb.on_issue();
    });

    in_cycle(2, [&]() {
        lsb.on_broadcast();
        lsb.on_issue();
        lsb.on_execute();
    });

    // Wait for memory access delay cycles
    for (int i = 3; i <= 5; i++) {
        in_cycle(i, [&]() {
            lsb.on_issue();
            lsb.on_execute();
        });
    }

    // Test 2: Store then Load (memory forwarding from LSB)
    std::cout << "\n--- Test 2: Store then Load (LSB Forwarding) ---" << std::endl;
    
    in_cycle(6, [&]() {
        // Store a byte at address 0x100
        auto store_entry = LSBTestUtils::create_resolver_entry(SB, 0x100, 0x42, 0, true, true, 2);
        chw.write(store_entry);
        lsb.on_issue();
    });

    in_cycle(7, [&]() {
        lsb.on_broadcast();
        lsb.on_issue();
        lsb.on_execute();  // Process store
    });

    in_cycle(8, [&]() {
        lsb.on_issue();
        lsb.on_execute();  // Complete store execution
    });

    in_cycle(9, [&]() {
        lsb.on_execute();  // Continue processing
        lsb.on_issue();
    });

    in_cycle(10, [&]() {
        // Load from the same address that was stored to
        auto load_entry = LSBTestUtils::create_resolver_entry(LB, 0x100, 0, 0, true, true, 3);
        chw.write(load_entry);
        lsb.on_issue();
        lsb.on_issue();
    });

    in_cycle(11, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();  // Should read from modification_list, not memory
    });

    // Wait for load completion
    for (int i = 12; i <= 15; i++) {
        in_cycle(i, [&]() {
            lsb.on_issue();
            lsb.on_execute();
        });
    }

    // Test 3: Multiple stores to different addresses
    std::cout << "\n--- Test 3: Multiple Stores (Sequence Testing) ---" << std::endl;
    
    in_cycle(16, [&]() {
        // Store word at address 0x200
        auto store1 = LSBTestUtils::create_resolver_entry(SW, 0x200, 0x12345678, 0, true, true, 4);
        chw.write(store1);
        lsb.on_issue();
    });

    in_cycle(17, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    in_cycle(18, [&]() {
        lsb.on_issue();
        lsb.on_execute();
    });

    in_cycle(19, [&]() {
        lsb.on_execute();
        
        // Store halfword at address 0x210
        auto store2 = LSBTestUtils::create_resolver_entry(SH, 0x210, 0xABCD, 0, true, true, 5);
        chw.write(store2);
        lsb.on_issue();
    });

    in_cycle(20, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    in_cycle(21, [&]() {
        lsb.on_issue();
        lsb.on_execute();
    });

    in_cycle(22, [&]() {
        lsb.on_execute();
        
        // Load word from first store location
        auto load1 = LSBTestUtils::create_resolver_entry(LW, 0x200, 0, 0, true, true, 6);
        chw.write(load1);
        lsb.on_issue();
    });

    in_cycle(23, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    // Test 4: Commit operations
    std::cout << "\n--- Test 4: Commit Operations ---" << std::endl;
    
    in_cycle(24, [&]() {
        // Commit the first store (rob_index 2)
        commit_bus.write(MockROBPointer::create_mock_pointer(2));
        lsb.on_issue();
        lsb.on_commit();
    });

    in_cycle(25, [&]() {
        // Commit the second store (rob_index 4)
        lsb.on_issue();
        commit_bus.write(MockROBPointer::create_mock_pointer(4));
        lsb.on_commit();
    });

    in_cycle(26, [&]() {
        // Commit the third store (rob_index 5)
        lsb.on_issue();
        commit_bus.write(MockROBPointer::create_mock_pointer(5));
        lsb.on_commit();
    });

    // Test 5: Reset functionality
    std::cout << "\n--- Test 5: Reset Functionality ---" << std::endl;
    
    in_cycle(27, [&]() {
        // Add some more stores before reset
        lsb.on_issue();
        auto store3 = LSBTestUtils::create_resolver_entry(SB, 0x300, 0x99, 0, true, true, 7);
        chw.write(store3);
        lsb.on_issue();
    });

    in_cycle(28, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    in_cycle(29, [&]() {
        lsb.on_issue();
        lsb.on_execute();
        
        // Issue reset signal
        ResetData reset_data(true, 0x2000);
        lsb.on_reset(reset_data);
        
        std::cout << "Reset signal issued - all uncommitted modifications should be cleared" << std::endl;
    });

    in_cycle(30, [&]() {
        // Try to load from address that had uncommitted store
        auto load_after_reset = LSBTestUtils::create_resolver_entry(LB, 0x300, 0, 0, true, true, 8);
        chw.write(load_after_reset);
        lsb.on_issue();
    });

    in_cycle(31, [&]() {
        lsb.on_broadcast();
        lsb.on_issue();
        lsb.on_execute();  // Should read from memory, not from cleared modification_list
    });

    // Test 6: Load different data sizes
    std::cout << "\n--- Test 6: Different Load Types ---" << std::endl;
    
    in_cycle(32, [&]() {
        // Test unsigned load byte
        auto load_lbu = LSBTestUtils::create_resolver_entry(LBU, 0x1000, 0, 0, true, true, 9);
        chw.write(load_lbu);
        lsb.on_issue();
    });

    in_cycle(33, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    in_cycle(34, [&]() {
        lsb.on_issue();
        lsb.on_execute();
        
        // Test load halfword
        auto load_lh = LSBTestUtils::create_resolver_entry(LH, 0x1000, 0, 0, true, true, 10);
        chw.write(load_lh);
        lsb.on_issue();
    });

    in_cycle(35, [&]() {
        lsb.on_issue();
        lsb.on_broadcast();
        lsb.on_execute();
    });

    // Wait for all operations to complete
    for (int i = 36; i <= 40; i++) {
        in_cycle(i, [&]() {
            lsb.on_issue();
            lsb.on_execute();
        });
    }

    std::cout << "\n=== Test Summary ===" << std::endl;
    if (failures == 0) {
        std::cout << "All LSB tests completed successfully!" << std::endl;
    } else {
        std::cout << "LSB tests completed with " << failures << " failures." << std::endl;
    }

    // Clean up test file
    std::remove(test_mem_file.c_str());

    return 0;
}
