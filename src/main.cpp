// use the perceptual hash
// hamming distance
// threads
// dynamic programing
// vp tree
             
#include <iostream>
#include "dpf/PHash.h"

int main() {
    PHash  phash{};

    std::cout << phash.getPHash("assets\\testImage\\IMG_20221203_185941043.jpg") << std::endl;
    std::cout << phash.getPHash("assets\\testImage\\IMG_20221203_1859410439.jpg") << std::endl;

    return 0;
}
