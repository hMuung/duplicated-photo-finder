#include "dpf/PHash.h"

PHash::PHash() {
    initCosTable();
}

PHash::~PHash() {}

// Save a cosine Look Up Table
void PHash::initCosTable() {
    constexpr float PI = 3.14159265358979323846f;

    for (int u = 0; u < 32; ++u) {
        for (int x = 0; x < 32; ++x) {
            cosTable[u][x] = ((u == 0) ? std::sqrt(1/32) : std::sqrt(2/32)) * std::cos(((2.0f*x + 1.0f)*u*PI) / 64.0f);
        }
    }
}

// Main pipeline to get the hash
uint64_t PHash::getPHash(std::string_view path) {
    auto img = readAndResize(path);
    auto dct = applyDCT(img);
    auto block = extractTopLeft8x8(dct);
    auto coeffs = flattenAndIgnoreDC(block);
    float median = computeMedian(coeffs);

    return generateBinaryHash(coeffs, median);
}

// Meant to read, resize and transform to gray scale
ImageGrid32 PHash::readAndResize(std::string_view path) {
    int width, height, channels;

    // Read image
    unsigned char* data = stbi_load(
        path.data(),
        &width,
        &height,
        &channels,
        0
    );

    if (!data) {
        throw std::runtime_error("Failed to load image");
    }

    // Nearest Neighbor resize method
    ImageGrid32 output{};

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            int srcY = (y * height) / 32;
            int srcX = (x * width) / 32;

            int idx = (srcY * width + srcX) * channels;

            float gray = 0.0f;

            if (channels == 1) {
                gray = static_cast<float>(data[srcY * width + srcX]);
            } else {
                uint8_t r = data[idx];
                uint8_t g = data[idx + 1];
                uint8_t b = data[idx + 2];
                
                // Gray scale transformation
                gray = 0.299f * r + 0.587f * g + 0.114f * b;
            }

            output[y][x] = gray;
        }
    }

    stbi_image_free(data);

    return output;
}

// Discrete Cosine Transform aplication
ImageGrid32 PHash::applyDCT(const ImageGrid32& input) {
    ImageGrid32 temp{};
    ImageGrid32 output{};

    // DCT rows
    for (int y = 0; y < 32; ++y) {
        for (int u = 0; u < 32; ++u) {
            float sum = 0.0f;

            for (int x = 0; x < 32; ++x) {
                sum += input[y][x] * cosTable[u][x];
            }
            temp[y][u] = sum;
        }
    }

    // DCT columns
    for (int v = 0; v < 32; ++v) {
        for (int x = 0; x < 32; ++x) {
            float sum = 0.0f;
            for (int y = 0; y < 32; ++y) {
                sum += temp[y][x] * cosTable[v][y];
            }
            output[v][x] = sum; 
        }
    }

    return output;
}

// Conserve only low frecuencies
ImageGrid8 PHash::extractTopLeft8x8(const ImageGrid32& dctMat) {
    ImageGrid8 block{};

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            block[y][x] = dctMat[y][x];
        }
    }

    return block;
}

// Ignores DC coeficient and changes the 2d array to a 1d array
CoeffArray PHash::flattenAndIgnoreDC(const ImageGrid8& block) {
    CoeffArray coeffs{};

    //A two-dimensional static array stores its data in contiguous memory space

    // Point to the first element of the matrix
    const float* src = &block[0][0];

    for (int i = 0; i < 63; ++i) {
        coeffs[i] = src[i + 1]; // i + 1 skip first element
    }

    return coeffs;
}

// Gives back the median value of the flatten coeficients
float PHash::computeMedian(CoeffArray values) {
    std::sort(values.begin(), values.end());
    return values[31];
}

// Generate binary with median threshold
uint64_t PHash::generateBinaryHash( const CoeffArray& coefficients, float median) {
    uint64_t hash = 0;

    for (int i = 0; i < 63; ++i) {
        if (coefficients[i] > median) {
            hash |= (1ULL << i);
        }
    }

    return hash;
}