#include <cstdint>
#include <iostream>

struct Order {
    uint64_t id;
    int32_t price;
    uint32_t qty;
    std::byte size;
};

int main() { std::cout << sizeof(Order) << " " << alignof(Order) << "\n"; }
