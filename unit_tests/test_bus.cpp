#include <cassert>

#include "utility/bus.hpp"

using namespace norb;

struct Component {
    Bus<int> bus;
};

int main() {
    Component a, b;
    Bus<int>::connect(a.bus, b.bus);
    a.bus.write(1);
    assert(a.bus.read() == 0);
    buffered_flush();
    assert(a.bus.read() == 1);
    assert(b.bus.read() == 1);
    buffered_flush();
    b.bus.write(2);
    assert(b.bus.read() == 1);
    buffered_flush();
    assert(a.bus.read() == 2);
    assert(b.bus.read() == 2);
    buffered_flush();
}