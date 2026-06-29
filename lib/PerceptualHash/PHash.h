#pragma once

/*

Pipeline
-> grayscale
-> resize to 32x32
-> DCT
-> top-left 8x8
-> remove DC
-> median
-> hash

*/

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
        // Resize image to 32×32
        ImageGrid readImage(std::string_view path);
        ColorImage readImage(std::string_view path);

        // Resize image to 32×32
        ImageGrid resizeImage(const ImageGrid& input);

        // Convert to grayscale if gray scale
        ImageGrid convertToGrayscale(const ImageGrid& input);

        // Convert to grayscale if color image
        ImageGrid convertToGrayscale(const ColorImage& input);

        // Convert to float
        ImageGrid convertToFloat(const ImageGrid& input);

        // Apply Descrete Cosine Transform
        ImageGrid applyDCT(const ImageGrid& input);

        // Keep top-left 8×8 block
        ImageGrid extractTopLeft8x8(const ImageGrid& dctMat);

        // Ignore the DC coefficient
        std::vector<float> flattenAndIgnoreDC(const ImageGrid& block8x8);

        // Compute median
        float computeMedian(std::vector<float> values);

        // Generate binary hash
        uint64_t generateBinaryHash(const std::vector<float>& coefficients, float median);

    public:
        PHash();
        ~PHash();

        uint64_t getPHash();

};

