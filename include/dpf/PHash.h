#pragma once

#include <array>
#include <cstdint>
#include <string_view>

using ImageGrid32 = std::array<std::array<float, 32>, 32>;
using ImageGrid8  = std::array<std::array<float, 8>, 8>;
using CoeffArray  = std::array<float, 63>;

class PHash {
private:
    float cosTable[32][32];

    void initCosTable();

    ImageGrid32 readAndResize(std::string_view path);
    ImageGrid32 applyDCT(const ImageGrid32& input);
    ImageGrid8 extractTopLeft8x8(const ImageGrid32& dctMat);

    CoeffArray flattenAndIgnoreDC(const ImageGrid8& block);
    float computeMedian(CoeffArray values);

    uint64_t generateBinaryHash(
        const CoeffArray& coefficients,
        float median
    );

public:
    PHash();
    ~PHash();

    uint64_t getPHash(std::string_view path);
};