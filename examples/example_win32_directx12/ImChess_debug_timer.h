#pragma once
#include <iostream>
#include <iomanip> 
#include <chrono>

class debug_timer
{
private:
    std::chrono::high_resolution_clock::time_point start;

public:
    debug_timer() {
        start = std::chrono::high_resolution_clock::now();
    }

    ~debug_timer() {
        const auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << duration << " ms" << std::endl;
    }
};
