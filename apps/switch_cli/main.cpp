#include "silicon_switch/version.hpp"

#include <iostream>

int main() {
    std::cout << "Silicon Switch Lab " << silicon_switch::version() << '\n';
    return 0;
}
