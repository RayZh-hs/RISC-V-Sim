#include <cassert>
#include <iostream>

#include "utility/buffered.hpp"

int main()
{
    norb::Buffered<int> a(0), b(1);
    norb::buffered_flush();
    assert(a.read() == 0);
    assert(b.read() == 1);
    a = 2;
    assert(a.read() == 0);
    norb::buffered_flush();
    assert(a.read() == 2);
    a.write(3);
    assert(a.read() == 2);
    norb::buffered_flush();
    assert(a.read() == 3);

    // a = 3, b = 1
    a = b.read();
    b = a.read();
    norb::buffered_flush();

    assert(static_cast<int>(a) == 1);
    assert(static_cast<int>(b) == 3);

    norb::buffered_flush();
    norb::buffered_flush();

    assert(static_cast<int>(a) == 1);
    assert(static_cast<int>(b) == 3);

    std::cout << "Test passed" << std::endl;
}