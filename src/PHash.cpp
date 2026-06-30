#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "dpf/PHash.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>

PHash::PHash() {
    initCosTable();
}

PHash::~PHash() {}

void PHash::initCosTable() {
    constexpr float PI = 3.14159265358979323846f;

    for (int u = 0; u < 32; ++u) {
        for (int x = 0; x < 32; ++x) {
            cosTable[u][x] =
                std::cos(((2.0f * x + 1.0f) * u * PI) / 64.0f);
        }
    }
}

uint64_t PHash::getPHash(std::string_view path) {
    auto img = readAndResize(path);
    auto dct = applyDCT(img);
    auto block = extractTopLeft8x8(dct);
    auto coeffs = flattenAndIgnoreDC(block);
    float median = computeMedian(coeffs);

    return generateBinaryHash(coeffs, median);
}

ImageGrid32 PHash::readAndResize(std::string_view path) {
    int width, height, channels;

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

                gray = 0.299f * r + 0.587f * g + 0.114f * b;
            }

            output[y][x] = gray;
        }
    }

    stbi_image_free(data);

    return output;
}

ImageGrid32 PHash::applyDCT(const ImageGrid32& input) {
    ImageGrid32 temp{};
    ImageGrid32 output{};

    // DCT filas
    for (int y = 0; y < 32; ++y) {
        for (int u = 0; u < 32; ++u) {
            float sum = 0.0f;

            for (int x = 0; x < 32; ++x) {
                sum += input[y][x] * cosTable[u][x];
            }

            float cu = (u == 0) ? std::sqrt(1.0f / 32.0f)
                                : std::sqrt(2.0f / 32.0f);

            temp[y][u] = cu * sum;
        }
    }

    // DCT columnas
    for (int x = 0; x < 32; ++x) {
        for (int v = 0; v < 32; ++v) {
            float sum = 0.0f;

            for (int y = 0; y < 32; ++y) {
                sum += temp[y][x] * cosTable[v][y];
            }

            float cv = (v == 0) ? std::sqrt(1.0f / 32.0f)
                                : std::sqrt(2.0f / 32.0f);

            output[v][x] = cv * sum;
        }
    }

    return output;
}

ImageGrid8 PHash::extractTopLeft8x8(const ImageGrid32& dctMat) {
    ImageGrid8 block{};

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            block[y][x] = dctMat[y][x];
        }
    }

    return block;
}

CoeffArray PHash::flattenAndIgnoreDC(const ImageGrid8& block) {
    CoeffArray coeffs{};

    int idx = 0;

    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 8; ++x) {
            if (y == 0 && x == 0)
                continue;

            coeffs[idx++] = block[y][x];
        }
    }

    return coeffs;
}

float PHash::computeMedian(CoeffArray values) {
    std::sort(values.begin(), values.end());
    return values[31];
}

uint64_t PHash::generateBinaryHash( const CoeffArray& coefficients, float median) {
    uint64_t hash = 0;

    for (int i = 0; i < 63; ++i) {
        if (coefficients[i] > median) {
            hash |= (1ULL << i);
        }
    }

    return hash;
}