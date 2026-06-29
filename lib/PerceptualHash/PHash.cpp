#include <PHash.h>

PHash::PHash() {}

PHash::~PHash() {}

uint64_t PHash::getPHash() {
    
}

// Resize image to 32×32
ImageGrid PHash::resizeImage(const ImageGrid& input) {
    // Resize Gray scale image to 32x32 size 
    // using nearest-neighbor interpolation

    const int newSize = 32;

    int oldHeight = input.size();
    int oldWidth = input[0].size();

    ImageGrid output(newSize, std::vector<float>(newSize));

    for (int y = 0; y < newSize; y++) {
        for (int x = 0; x < newSize; x++) {
            int srcY = static_cast<int>((float)y * oldHeight / newSize);
            int srcX = static_cast<int>((float)x * oldWidth / newSize);

            output[y][x] = input[srcY][srcX];
        }
    }

    return output;

}

// Convert to grayscale if gray scale
ImageGrid PHash::convertToGrayscale(const ImageGrid& input) {
    // Only return input

    return input;
}

// Convert to grayscale if color image
ImageGrid PHash::convertToGrayscale(const ColorImage& input) {
    // Use standar formula to convert to gray scale
    // Y = 0.299R + 0.587G + 0.114B

    int height = input.size();
    int width = input[0].size();

    ImageGrid grayScale(height, std::vector<float>(width));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const Pixel& p = input[y][x];
            grayScale[y][x] = 0.299f*p.r + 0.587f*p.g + 0.114f*p.b;
        }
    }

    return grayScale;
}

// Convert to float
ImageGrid PHash::convertToFloat(const ImageGrid& input) {

}

// Apply DCT
ImageGrid PHash::applyDCT(const ImageGrid& input) {

}

// Keep top-left 8×8 block
ImageGrid PHash::extractTopLeft8x8(const ImageGrid& dctMat) {

}

// Ignore the DC coefficient
std::vector<float> PHash::flattenAndIgnoreDC(const ImageGrid& block8x8) {

}

// Compute median
float PHash::computeMedian(std::vector<float> values) {

}

// Generate binary hash
uint64_t PHash::generateBinaryHash(const std::vector<float>& coefficients, float median) {

}