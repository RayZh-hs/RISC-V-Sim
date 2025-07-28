#include <cassert>
#include <functional>
#include <iostream>
#include <memory>

#include "alu.hpp"
#include "cdb.hpp"
#include "decoder.hpp"
#include "rob_types.hpp"
#include "rs.hpp"
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

// Test utilities
class ALUTestUtils {
public:
    static ResolvedInstructionEntry create_test_entry(InsType type, uint32_t vj = 0, uint32_t vk = 0,
                                                      uint32_t imm = 0) {
        ResolvedInstructionEntry entry;
        entry.type = type;
        entry.vj = vj;
        entry.vk = vk;
        entry.imm = imm;
        entry.rob_pointer = MockROBPointer::create_mock_pointer();
        entry.pc = 0x1000;
        entry.had_jumped = false;
        return entry;
    }

    static ResolverEntry create_resolver_entry(InsType type, uint32_t vj = 0, uint32_t vk = 0, uint32_t imm = 0,
                                               bool j_ready = true, bool k_ready = true) {
        ResolverEntry entry;
        entry.type = type;
        entry.status = (j_ready && k_ready) ? ResolverEntryStatus::READY : ResolverEntryStatus::PENDING;
        entry.rob_pointer = MockROBPointer::create_mock_pointer();
        entry.k_is_ready = k_ready;
        entry.j_is_ready = j_ready;
        entry.vk = vk;
        entry.vj = vj;
        entry.imm = imm;
        entry.pc = 0x1000;
        entry.had_jumped = false;
        return entry;
    }
};

void in_cycle(int cycle_number, const std::function<void()>& cycle_operations) {
    std::cout << "Cycle " << cycle_number << std::endl;
    cycle_operations();
    norb::buffered_flush();
    norb::Clock::instance().tick();
}

int main() {
    Logger::get().setLevel(LogLevel::DEBUG);
    norb::Clock::instance().reset();

    ReservationStation rs;
    auto shared_cdb = std::make_shared<CommonDataBus>();
    norb::ChannelWriter<ResolverEntry> chw;
    rs.resolver.load_cdb(shared_cdb);

    int failures = 0;
    auto ASSERT_TRUE = [&](bool condition, const std::string& test_name) {
        if (!condition) {
            std::cerr << "FAIL: " << test_name << std::endl;
            failures++;
        } else {
            std::cout << "PASS: " << test_name << std::endl;
        }
    };

    rs.resolver.bind_inbound_to(chw);

    in_cycle(1, [&]() {
        ResolverEntry entry1;
        entry1.status = ResolverEntryStatus::PENDING;
        entry1.type = ADD;
        entry1.j_is_ready = false;
        entry1.k_is_ready = false;
        entry1.rob_pointer = MockROBPointer::create_mock_pointer(3);
        entry1.qj = MockROBPointer::create_mock_pointer(1);
        entry1.qk = MockROBPointer::create_mock_pointer(2);

        chw.write(entry1);
        rs.on_issue();
    });

    in_cycle(2, [&]() {
        rs.on_issue();
        rs.on_execute();
    });

    in_cycle(3, [&]() {
        ResolverEntry entry2;
        entry2.status = ResolverEntryStatus::PENDING;
        entry2.type = ADDI;
        entry2.j_is_ready = false;
        entry2.k_is_ready = true;
        entry2.imm = 10;
        entry2.rob_pointer = MockROBPointer::create_mock_pointer(4);
        entry2.qj = MockROBPointer::create_mock_pointer(3);
        chw.write(entry2);
    });

    in_cycle(4, [&]() { rs.resolver.listen_inbound(); });

    in_cycle(5, [&]() {
        // broadcast success for the two dependencies
        shared_cdb->broadcast({MockROBPointer::create_mock_pointer(1), 5});
        rs.resolver.listen_inbound();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(6, [&]() {
        shared_cdb->broadcast({MockROBPointer::create_mock_pointer(2), 6});
        rs.resolver.listen_inbound();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(7, [&]() {
        rs.resolver.listen_inbound();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(8, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(9, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(10, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(11, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(12, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(13, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(14, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });

    in_cycle(15, [&]() {
        rs.on_issue();
        rs.on_broadcast();
        rs.on_execute();
        shared_cdb->flush();
    });
}
