#include "../include/hash_functions.h"
#include <cstring>

uint64_t murmurHash3(const std::string& key) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key.data());
    size_t len = key.size();

    uint64_t h1 = 0;
    uint64_t h2 = 0;

    const uint64_t c1 = 0x87c37b91114253d5ULL;
    const uint64_t c2 = 0x4cf5ad432745937fULL;

    size_t blocks = len / 16;

    for (size_t i = 0; i < blocks; i++) {
        uint64_t k1, k2;

        std::memcpy(&k1, data + i * 16, 8);
        std::memcpy(&k2, data + i * 16 + 8, 8);

        k1 *= c1;
        k1 = (k1 << 31) | (k1 >> 33);
        k1 *= c2;
        h1 ^= k1;

        h1 = (h1 << 27) | (h1 >> 37);
        h1 += h2;
        h1 = h1 * 5 + 0x52dce729;

        k2 *= c2;
        k2 = (k2 << 33) | (k2 >> 31);
        k2 *= c1;
        h2 ^= k2;

        h2 = (h2 << 31) | (h2 >> 33);
        h2 += h1;
        h2 = h2 * 5 + 0x38495ab5;
    }

    uint64_t k1 = 0;
    uint64_t k2 = 0;

    const uint8_t* tail = data + blocks * 16;
    size_t remaining = len & 15;

    switch (remaining) {
        case 15: k2 ^= static_cast<uint64_t>(tail[14]) << 48;
        case 14: k2 ^= static_cast<uint64_t>(tail[13]) << 40;
        case 13: k2 ^= static_cast<uint64_t>(tail[12]) << 32;
        case 12: k2 ^= static_cast<uint64_t>(tail[11]) << 24;
        case 11: k2 ^= static_cast<uint64_t>(tail[10]) << 16;
        case 10: k2 ^= static_cast<uint64_t>(tail[9]) << 8;
        case 9:  k2 ^= static_cast<uint64_t>(tail[8]);
                 k2 *= c2;
                 k2 = (k2 << 33) | (k2 >> 31);
                 k2 *= c1;
                 h2 ^= k2;

        case 8:  k1 ^= static_cast<uint64_t>(tail[7]) << 56;
        case 7:  k1 ^= static_cast<uint64_t>(tail[6]) << 48;
        case 6:  k1 ^= static_cast<uint64_t>(tail[5]) << 40;
        case 5:  k1 ^= static_cast<uint64_t>(tail[4]) << 32;
        case 4:  k1 ^= static_cast<uint64_t>(tail[3]) << 24;
        case 3:  k1 ^= static_cast<uint64_t>(tail[2]) << 16;
        case 2:  k1 ^= static_cast<uint64_t>(tail[1]) << 8;
        case 1:  k1 ^= static_cast<uint64_t>(tail[0]);
                 k1 *= c1;
                 k1 = (k1 << 31) | (k1 >> 33);
                 k1 *= c2;
                 h1 ^= k1;
    }

    h1 ^= len;
    h2 ^= len;

    h1 += h2;
    h2 += h1;

    h1 ^= h1 >> 33;
    h1 *= 0xff51afd7ed558ccdULL;
    h1 ^= h1 >> 33;
    h1 *= 0xc4ceb9fe1a85ec53ULL;
    h1 ^= h1 >> 33;

    h2 ^= h2 >> 33;
    h2 *= 0xff51afd7ed558ccdULL;
    h2 ^= h2 >> 33;
    h2 *= 0xc4ceb9fe1a85ec53ULL;
    h2 ^= h2 >> 33;

    h1 += h2;

    return h1;
}

uint64_t fnv1aAvalancheHash(const std::string& key) {
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (unsigned char c : key) {
        hash ^= c;
        hash *= 0x100000001b3ULL;
    }

    hash ^= hash >> 32;
    hash *= 0x9e3779b185ebca87ULL;
    hash ^= hash >> 29;

    return hash;
}

uint64_t xxHash(const std::string& key) {
    // Canonical XXH64 algorithm, seed=0, little-endian input interpretation.
    constexpr uint64_t P1 = 11400714785074694791ULL;
    constexpr uint64_t P2 = 14029467366897019727ULL;
    constexpr uint64_t P3 = 1609587929392839161ULL;
    constexpr uint64_t P4 = 9650029242287828579ULL;
    constexpr uint64_t P5 = 2870177450012600261ULL;
    auto read32 = [](const unsigned char* p) -> uint32_t {
        return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
    };
    auto read64 = [](const unsigned char* p) -> uint64_t {
        uint64_t v=0; for (unsigned i=0;i<8;++i) v |= static_cast<uint64_t>(p[i]) << (8*i); return v;
    };
    auto round = [](uint64_t acc, uint64_t input) -> uint64_t {
        acc += input * P2; acc = (acc << 31) | (acc >> 33); return acc * P1;
    };
    auto mergeRound = [&](uint64_t acc, uint64_t v) -> uint64_t {
        acc ^= round(0, v); return acc * P1 + P4;
    };
    const auto* p = reinterpret_cast<const unsigned char*>(key.data());
    const auto* const end = p + key.size();
    const size_t len = key.size();
    uint64_t h;
    if (len >= 32) {
        uint64_t v1=P1+P2, v2=P2, v3=0, v4=0-P1;
        const auto* const limit=end-32;
        do { v1=round(v1,read64(p)); p+=8; v2=round(v2,read64(p)); p+=8;
             v3=round(v3,read64(p)); p+=8; v4=round(v4,read64(p)); p+=8; } while(p<=limit);
        h=((v1<<1)|(v1>>63))+((v2<<7)|(v2>>57))+((v3<<12)|(v3>>52))+((v4<<18)|(v4>>46));
        h=mergeRound(h,v1); h=mergeRound(h,v2); h=mergeRound(h,v3); h=mergeRound(h,v4);
    } else h=P5;
    h += len;
    while (p+8 <= end) { uint64_t k=round(0,read64(p)); h^=k; h=((h<<27)|(h>>37))*P1+P4; p+=8; }
    if (p+4 <= end) { h^=static_cast<uint64_t>(read32(p))*P1; h=((h<<23)|(h>>41))*P2+P3; p+=4; }
    while (p<end) { h^=static_cast<uint64_t>(*p)*P5; h=((h<<11)|(h>>53))*P1; ++p; }
    h^=h>>33; h*=P2; h^=h>>29; h*=P3; h^=h>>32;
    return h;
}

uint64_t getHash(const std::string& key, HashType type) {
    switch (type) {
        case HashType::MURMURHASH3:
            return murmurHash3(key);

        case HashType::FNV1A_AVALANCHE:
            return fnv1aAvalancheHash(key);

        case HashType::XXHASH:
            return xxHash(key);
    }

    return 0;
}

std::string hashTypeToString(HashType type) {
    switch (type) {
        case HashType::MURMURHASH3:
            return "MurmurHash3 x64/128 (low 64 bits)";

        case HashType::FNV1A_AVALANCHE:
            return "FNV-1a + avalanche";

        case HashType::XXHASH:
            return "xxHash";
    }

    return "Unknown";
}