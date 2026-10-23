// Minimal baseline JPEG encoder (JFIF, 4:2:0, standard Huffman tables) for
// camera BLOB streams: vendor code cannot link the system libjpeg.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace jpeg {

namespace detail {

const uint8_t kZigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};
const uint8_t kLumaQ[64] = {
    16, 11, 10, 16, 24,  40,  51,  61,  12, 12, 14, 19, 26,  58,  60,  55,
    14, 13, 16, 24, 40,  57,  69,  56,  14, 17, 22, 29, 51,  87,  80,  62,
    18, 22, 37, 56, 68,  109, 103, 77,  24, 35, 55, 64, 81,  104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};
const uint8_t kChromaQ[64] = {
    17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};
// Annex K standard tables: code counts per length 1..16, then symbols.
const uint8_t kDcLumaBits[16] = {0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};
const uint8_t kDcChromaBits[16] = {0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0};
const uint8_t kDcVals[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
const uint8_t kAcLumaBits[16] = {0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d};
const uint8_t kAcLumaVals[162] = {
    0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61,
    0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52,
    0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25,
    0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63, 0x64,
    0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
    0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
    0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
    0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3,
    0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8,
    0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
const uint8_t kAcChromaBits[16] = {0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77};
const uint8_t kAcChromaVals[162] = {
    0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61,
    0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33,
    0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18,
    0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x63,
    0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
    0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
    0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca,
    0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};

struct Huffman {
    uint16_t code[256] = {};
    uint8_t size[256] = {};
    Huffman(const uint8_t bits[16], const uint8_t* vals) {
        uint16_t c = 0;
        int k = 0;
        for (int len = 1; len <= 16; len++) {
            for (int i = 0; i < bits[len - 1]; i++, k++) {
                code[vals[k]] = c++;
                size[vals[k]] = uint8_t(len);
            }
            c <<= 1;
        }
    }
};

class Writer {
  public:
    std::vector<uint8_t> out;
    void byte(uint8_t b) { out.push_back(b); }
    void word(uint16_t w) { byte(uint8_t(w >> 8)); byte(uint8_t(w)); }
    void bits(uint32_t value, int count) {
        acc_ = (acc_ << count) | (value & ((1u << count) - 1));
        n_ += count;
        while (n_ >= 8) {
            uint8_t b = uint8_t(acc_ >> (n_ - 8));
            byte(b);
            if (b == 0xff) byte(0);  // byte stuffing
            n_ -= 8;
        }
    }
    void flush() { if (n_ > 0) bits(0x7f, 8 - n_); }
  private:
    uint32_t acc_ = 0;
    int n_ = 0;
};

inline void fdct(float* d) {
    // Separable float DCT-II (8x8): plenty for 30 fps snapshots.
    static float c[8][8];
    static bool init = false;
    if (!init) {
        for (int u = 0; u < 8; u++)
            for (int x = 0; x < 8; x++)
                c[u][x] = (u == 0 ? std::sqrt(0.125f) : 0.5f) *
                          std::cos((2 * x + 1) * u * float(M_PI) / 16);
        init = true;
    }
    float t[64];
    for (int y = 0; y < 8; y++)
        for (int u = 0; u < 8; u++) {
            float s = 0;
            for (int x = 0; x < 8; x++) s += c[u][x] * d[y * 8 + x];
            t[y * 8 + u] = s;
        }
    for (int u = 0; u < 8; u++)
        for (int v = 0; v < 8; v++) {
            float s = 0;
            for (int y = 0; y < 8; y++) s += c[v][y] * t[y * 8 + u];
            d[v * 8 + u] = s;
        }
}

inline int category(int v) {
    int a = v < 0 ? -v : v, n = 0;
    while (a) { n++; a >>= 1; }
    return n;
}

inline void block(Writer& w, float* d, const float* q, int& dc, const Huffman& hdc,
                  const Huffman& hac) {
    fdct(d);
    int z[64];
    for (int i = 0; i < 64; i++) z[i] = int(std::lround(d[kZigzag[i]] / q[kZigzag[i]]));
    int diff = z[0] - dc;
    dc = z[0];
    int cat = category(diff);
    w.bits(hdc.code[cat], hdc.size[cat]);
    if (cat) w.bits(uint32_t(diff < 0 ? diff - 1 : diff), cat);
    int run = 0;
    for (int i = 1; i < 64; i++) {
        if (!z[i]) { run++; continue; }
        while (run > 15) { w.bits(hac.code[0xf0], hac.size[0xf0]); run -= 16; }
        cat = category(z[i]);
        int sym = (run << 4) | cat;
        w.bits(hac.code[sym], hac.size[sym]);
        w.bits(uint32_t(z[i] < 0 ? z[i] - 1 : z[i]), cat);
        run = 0;
    }
    if (run) w.bits(hac.code[0], hac.size[0]);
}

inline void dht(Writer& w, int cls, int id, const uint8_t bits[16], const uint8_t* vals) {
    int n = 0;
    for (int i = 0; i < 16; i++) n += bits[i];
    w.word(0xffc4);
    w.word(uint16_t(3 + 16 + n));
    w.byte(uint8_t((cls << 4) | id));
    for (int i = 0; i < 16; i++) w.byte(bits[i]);
    for (int i = 0; i < n; i++) w.byte(vals[i]);
}

}  // namespace detail

// Encode an NV12 image (Y plane then interleaved CbCr, both with `stride`).
inline std::vector<uint8_t> encodeNv12(const uint8_t* y, const uint8_t* uv, int width,
                                       int height, int stride, int quality) {
    using namespace detail;
    quality = quality < 1 ? 1 : quality > 100 ? 100 : quality;
    const int scale = quality < 50 ? 5000 / quality : 200 - quality * 2;
    uint8_t ql[64], qc[64];
    float fl[64], fc[64];
    for (int i = 0; i < 64; i++) {
        int a = (kLumaQ[i] * scale + 50) / 100, b = (kChromaQ[i] * scale + 50) / 100;
        ql[i] = uint8_t(a < 1 ? 1 : a > 255 ? 255 : a);
        qc[i] = uint8_t(b < 1 ? 1 : b > 255 ? 255 : b);
        fl[i] = ql[i];
        fc[i] = qc[i];
    }
    static const Huffman dcl(kDcLumaBits, kDcVals), dcc(kDcChromaBits, kDcVals),
        acl(kAcLumaBits, kAcLumaVals), acc(kAcChromaBits, kAcChromaVals);
    Writer w;
    w.out.reserve(size_t(width) * height / 4);
    w.word(0xffd8);
    const uint8_t app0[] = {0x00, 0x10, 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0};
    w.word(0xffe0);
    for (uint8_t b : app0) w.byte(b);
    for (int t = 0; t < 2; t++) {
        w.word(0xffdb);
        w.word(67);
        w.byte(uint8_t(t));
        for (int i = 0; i < 64; i++) w.byte(t ? qc[kZigzag[i]] : ql[kZigzag[i]]);
    }
    w.word(0xffc0);
    w.word(17);
    w.byte(8);
    w.word(uint16_t(height));
    w.word(uint16_t(width));
    w.byte(3);
    const uint8_t comps[9] = {1, 0x22, 0, 2, 0x11, 1, 3, 0x11, 1};
    for (uint8_t b : comps) w.byte(b);
    dht(w, 0, 0, kDcLumaBits, kDcVals);
    dht(w, 1, 0, kAcLumaBits, kAcLumaVals);
    dht(w, 0, 1, kDcChromaBits, kDcVals);
    dht(w, 1, 1, kAcChromaBits, kAcChromaVals);
    w.word(0xffda);
    w.word(12);
    w.byte(3);
    const uint8_t sos[9] = {1, 0x00, 2, 0x11, 3, 0x11, 0, 63, 0};
    for (uint8_t b : sos) w.byte(b);

    int dcY = 0, dcU = 0, dcV = 0;
    float d[64];
    auto clampX = [&](int x) { return x < width ? x : width - 1; };
    auto clampY = [&](int v) { return v < height ? v : height - 1; };
    for (int my = 0; my < height; my += 16) {
        for (int mx = 0; mx < width; mx += 16) {
            for (int by = 0; by < 2; by++)
                for (int bx = 0; bx < 2; bx++) {
                    for (int r = 0; r < 8; r++)
                        for (int c = 0; c < 8; c++)
                            d[r * 8 + c] = float(y[size_t(clampY(my + by * 8 + r)) * stride +
                                                   clampX(mx + bx * 8 + c)]) - 128.0f;
                    block(w, d, fl, dcY, dcl, acl);
                }
            for (int plane = 0; plane < 2; plane++) {
                for (int r = 0; r < 8; r++)
                    for (int c = 0; c < 8; c++) {
                        int sy = clampY(my + r * 2) / 2, sx = clampX(mx + c * 2) / 2;
                        d[r * 8 + c] = float(uv[size_t(sy) * stride + sx * 2 + plane]) - 128.0f;
                    }
                block(w, d, fc, plane ? dcV : dcU, dcc, acc);
            }
        }
    }
    w.flush();
    w.word(0xffd9);
    return std::move(w.out);
}

}  // namespace jpeg
