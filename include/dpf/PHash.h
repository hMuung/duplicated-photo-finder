#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <optional>
#include <string>

/*
PHash
Generates a 64-bit Perceptual Hash (pHash) of an image for similarity comparison

This module resizes the input image, converts it to grayscale, applies a 2D Discrete 
Cosine Transform (DCT), and extracts the low-frequency coefficients to build a robust hash.

In case of failure (e.g., file not found or invalid format), the error is logged 
via the internal Logger and the function returns 0ULL
 */

using ImageGrid32 = std::array<std::array<float, 32>, 32>;
using ImageGrid8  = std::array<std::array<float, 8>, 8>;
using CoeffArray  = std::array<float, 63>;

class PHash {
    private:
        float cosTable[32][32];

        void initCosTable();

        std::optional<ImageGrid32> readAndResize(const char* path, std::string& errorOut);
        
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

        uint64_t getPHash(const char* path);
};