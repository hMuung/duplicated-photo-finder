#include <PHash.h>

PHash::PHash() {}

PHash::~PHash() {}

// Resize image to 32×32
ImageGrid PHash::resizeImage(const ImageGrid& input) {
    
}

// Convert to grayscale
ImageGrid PHash::convertToGrayscale(const ImageGrid& input) {

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