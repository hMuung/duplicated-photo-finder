// use the perceptual hash
// hamming distance
// threads
// dynamic programing
// vp tree
             
#include <iostream>
#include <chrono>
#include <cstdint>

#include "dpf/PHash.h"


int main() {
    PHash  phash{};

    const char* path = "assets\\testImage\\IMG_20221203_1859410439.jpg";

    auto inicio = std::chrono::high_resolution_clock::now();

    uint64_t hash = phash.getPHash(path);

    auto fin = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> tiempo = fin - inicio;

    std::cout << "Total time: " << tiempo.count() << " ms" << std::endl;
    std::cout << "Hash: " << hash << std::endl;

    return 0;

}
