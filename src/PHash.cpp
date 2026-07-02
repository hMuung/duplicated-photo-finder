#include "dpf/PHash.h"
#include "dpf/Logger.h"

#include <cmath>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb/stb_image_resize2.h"

PHash::PHash() {
    initCosTable();
}

PHash::~PHash() {}

// Save a cosine Look Up Table
void PHash::initCosTable() {
    constexpr float PI = 3.14159265358979323846f;

    for (int u = 0; u < 32; ++u) {
        for (int x = 0; x < 32; ++x) {
            cosTable[u][x] = std::cos(((2.0f*x + 1.0f)*u*PI) / 64.0f);
        }
    }
}

// Main pipeline to get the hash
uint64_t PHash::getPHash(const char* path) {
    std::string errorMessage;

    auto img = readAndResize(path, errorMessage);

    if (!img.has_value()) {
        // Send message to log
        Logger::error(errorMessage);
        // Return 0 to indicate failure
        return 0ULL; 
    }

    PHash::ImageGrid32 dct = applyDCT(img.value());
    PHash::ImageGrid8 block = extractTopLeft8x8(dct);
    PHash::CoeffArray coeffs = flattenAndIgnoreDC(block);
    float median = computeMedian(coeffs);

    return generateBinaryHash(coeffs, median);
}

// Reads, resizes, and converts the image to grayscale
std::optional<PHash::ImageGrid32> PHash::readAndResize(const char* path, std::string& errorOut) {
    int width, height, channels;
    
    // Force STB to load imagen in 1 canal at loading
    unsigned char* data = stbi_load(path, &width, &height, &channels, 1);

    if (!data) {
        // Save specific error
        errorOut = "Failed to load image [" + std::string(path) + "]: STBI could not read the file.";
        return std::nullopt; // Return a void optional indicating failure
    }

    // Create 32x32 bytes temporal buffer
    unsigned char resizedData[32 * 32];

    // Resize
    stbir_resize_uint8_linear(data, width, height, 0, resizedData, 32, 32, 0, STBIR_1CHANNEL);

    // Free Image from memory
    stbi_image_free(data);

    // Save data in ImageGrid32 format
    PHash::ImageGrid32 output{};
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            output[y][x] = static_cast<float>(resizedData[y * 32 + x]);
        }
    }

    return output;
}

// Discrete Cosine Transform application
PHash::ImageGrid32 PHash::applyDCT(const ImageGrid32& input) {
    PHash::ImageGrid32 temp{};
    PHash::ImageGrid32 output{};

    // DCT rows
    for (int y = 0; y < 32; ++y) {
        for (int u = 0; u < 32; ++u) {
            float sum = 0.0f;

            for (int x = 0; x < 32; ++x) {
                sum += input[y][x] * cosTable[u][x];
            }

            float cu = (u == 0) ? std::sqrt(1.0f / 32.0f) : std::sqrt(2.0f / 32.0f);

            temp[y][u] = cu * sum;
        }
    }

    // DCT columns
    for (int x = 0; x < 32; ++x) {
        for (int v = 0; v < 32; ++v) {
            float sum = 0.0f;

            for (int y = 0; y < 32; ++y) {
                sum += temp[y][x] * cosTable[v][y];
            }

            float cv = (v == 0) ? std::sqrt(1.0f / 32.0f) : std::sqrt(2.0f / 32.0f);

            output[v][x] = cv * sum;
        }
    }

    return output;
}

// Extract top-left 8x8 low frequencies
PHash::ImageGrid8 PHash::extractTopLeft8x8(const PHash::ImageGrid32& dctMat) {
    PHash::ImageGrid8 block{};

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            block[y][x] = dctMat[y][x];
        }
    }

    return block;
}

// Ignores DC coefficient and changes the 2d array to a 1d array
PHash::CoeffArray PHash::flattenAndIgnoreDC(const PHash::ImageGrid8& block) {
    PHash::CoeffArray coeffs{};

    //A two-dimensional static array stores its data in contiguous memory space

    // Point to the first element of the matrix
    const float* src = &block[0][0];

    for (int i = 0; i < 63; ++i) {
        coeffs[i] = src[i + 1]; // i + 1 skip first element
    }

    return coeffs;
}

// Gives back the median value of the flattened coefficients
float PHash::computeMedian(PHash::CoeffArray values) {
    std::sort(values.begin(), values.end());
    return values[31];
}

// Generate binary with median threshold
uint64_t PHash::generateBinaryHash( const PHash::CoeffArray& coefficients, float median) {
    uint64_t hash = 0;

    for (int i = 0; i < 63; ++i) {
        if (coefficients[i] > median) {
            hash |= (1ULL << i);
        }
    }

    return hash;
}
