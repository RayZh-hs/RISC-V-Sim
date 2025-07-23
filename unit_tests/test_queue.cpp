#include <iostream>
#include <third_party/queue.hpp>

void print_queue(const norb::FixedQueue<int, 5>& q, const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
    std::cout << "Contents: ";
    for (const auto& item : q) {
        std::cout << item << " ";
    }
    std::cout << "\n";
}

int main() {
    norb::FixedQueue<int, 5> q;
    q.push(10);
    q.push(20);
    q.push(30);
    q.push(40);

    print_queue(q, "Initial Queue");

    // Get an iterator to the second element (20)
    auto it = q.begin() + 1;
    std::cout << "Iterator 'it' points to: " << *it << std::endl;

    // Pop the front element (10)
    q.pop();
    print_queue(q, "After popping front (10)");

    // The iterator 'it' still points to the same physical memory slot,
    // which still contains the value 20.
    // Its logical position has changed from 1 to 0.
    std::cout << "After pop, 'it' still points to: " << *it << std::endl;
    std::cout << "Is 'it' now the same as begin()? " << std::boolalpha << (it == q.begin()) << std::endl;

    // Push two new elements, causing the buffer to wrap around
    q.push(50);
    q.push(60); // This overwrites the slot that originally held 10
    print_queue(q, "After pushing 50 and 60");

    // 'it' is unaffected
    std::cout << "After more pushes, 'it' still points to: " << *it << std::endl;

    // Now, let's pop until 'it' becomes invalid
    q.pop(); // pops 20
    print_queue(q, "After popping 20");

    std::cout << "The element at the iterator's physical location has been popped." << std::endl;
    std::cout << "Dereferencing 'it' now is UNDEFINED BEHAVIOR." << std::endl;
    // The following line would likely crash or read garbage data if the assert is disabled.
    // std::cout << "Value of dangling iterator: " << *it << std::endl;

    return 0;
}