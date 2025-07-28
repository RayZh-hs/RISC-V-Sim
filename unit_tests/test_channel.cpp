#include <cassert>
#include <iostream>
#include <stdexcept>

#include "utility/chan.hpp"

int main() {
    std::cout << "Testing Channel implementation..." << std::endl;

    // Test 1: Basic channel creation and initial state
    {
        std::cout << "Test 1: Basic channel creation and initial state" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        norb::make_channel(writer, reader);
        
        // Initially, channel should have no data
        assert(!reader.has_data());
        assert(!writer.has_data());
        std::cout << "✓ Channel created with no initial data" << std::endl;
    }

    // Test 2: Basic write and read operations
    {
        std::cout << "Test 2: Basic write and read operations" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        norb::make_channel(writer, reader);
        
        // Write data
        writer.write(42);
        norb::buffered_flush();
        assert(reader.has_data());
        assert(writer.has_data());
        
        // Read data
        int value = reader.read();
        norb::buffered_flush();
        assert(value == 42);
        assert(!reader.has_data());
        assert(!writer.has_data());
        std::cout << "✓ Basic write and read operations work correctly" << std::endl;
    }

    // Test 3: Prevent double write (abort double write)
    {
        std::cout << "Test 3: Prevent double write" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        make_channel(writer, reader);
        
        // First write should succeed
        writer.write(100);
        norb::buffered_flush();
        assert(writer.has_data());
        
        // Second write should throw exception
        bool exception_thrown = false;
        try {
            writer.write(200);
            norb::buffered_flush();
        } catch (const std::runtime_error& e) {
            exception_thrown = true;
            assert(std::string(e.what()) == "Cannot write: data already available");
        }
        assert(exception_thrown);
        
        // Data should still be the first value
        assert(reader.read() == 100);
        norb::buffered_flush();

        std::cout << "✓ Double write prevention works correctly" << std::endl;
    }

    // Test 4: Prevent read when no data available
    {
        std::cout << "Test 4: Prevent read when no data available" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        norb::make_channel(writer, reader);
        
        // Reading from empty channel should throw exception
        bool exception_thrown = false;
        try {
            reader.read();
        } catch (const std::runtime_error& e) {
            exception_thrown = true;
            assert(std::string(e.what()) == "No data available to read");
        }
        assert(exception_thrown);
        std::cout << "✓ Read prevention when no data works correctly" << std::endl;
    }

    // Test 5: Monitor consumption - after read, channel becomes available for write
    {
        std::cout << "Test 5: Monitor consumption - write after read" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        norb::make_channel(writer, reader);
        
        // Write, read, then write again should work
        writer.write(10);
        norb::buffered_flush();
        assert(reader.has_data());
        
        int value1 = reader.read();
        norb::buffered_flush();
        assert(value1 == 10);
        assert(!reader.has_data());
        
        // Now we should be able to write again
        writer.write(20);
        norb::buffered_flush();
        assert(reader.has_data());
        
        int value2 = reader.read();
        assert(value2 == 20);
        norb::buffered_flush();
        assert(!reader.has_data());
        std::cout << "✓ Consumption monitoring works correctly" << std::endl;
    }

    // Test 7: Multiple write-read cycles
    {
        std::cout << "Test 7: Multiple write-read cycles" << std::endl;
        norb::ChannelReader<int> reader;
        norb::ChannelWriter<int> writer;
        
        norb::make_channel(writer, reader);
        
        for (int i = 0; i < 10; ++i) {
            assert(!reader.has_data());
            writer.write(i * i);
            norb::buffered_flush();
            assert(reader.has_data());
            int value = reader.read();
            norb::buffered_flush();
            assert(value == i * i);
            assert(!reader.has_data());
        }
        std::cout << "✓ Multiple write-read cycles work correctly" << std::endl;
    }

    std::cout << "All tests passed!" << std::endl;
    return 0;
}