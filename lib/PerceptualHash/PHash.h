#pragma once

#include <vector>
#include <cstdint>
#include <cmath>

// Define ImageGrid as a vector of vectors with floating numbers (2D matriz)
using ImageGrid = std::vector<std::vector<float>>;

// Pixel Structure
struct Pixel {
    uint8_t r, g, b;
};

// Define ColorImage as a vector of vectors with pixels (3D matriz)
using ColorImage = std::vector<std::vector<Pixel>>;

class PHash {
    private:
        // Step 1: Resize image to 32×32
        ImageGrid resizeImage(const ImageGrid& input);

        // Step 2: Convert to grayscale
        ImageGrid convertToGrayscale(const ImageGrid& input);

        // Step 3: Convert to float
        ImageGrid convertToFloat(const ImageGrid& input);

        // Step 4: Apply DCT
        ImageGrid applyDCT(const ImageGrid& input);

        // Step 5: Keep top-left 8×8 block
        ImageGrid extractTopLeft8x8(const ImageGrid& dctMat);

        // Step 6: Ignore the DC coefficient
        std::vector<float> flattenAndIgnoreDC(const ImageGrid& block8x8);

        // Step 7: Compute median
        float computeMedian(std::vector<float> values);

        // Step 8: Generate binary hash
        uint64_t generateBinaryHash(const std::vector<float>& coefficients, float median);

    public:
        PHash();
        ~PHash();
};

