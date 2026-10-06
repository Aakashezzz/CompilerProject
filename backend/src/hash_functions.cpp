#include "../include/hash_functions.h"
#include <cstring>

uint64_t murmurHash3(const std::string& key) {
    const uint8_t* data = reinterpret_cast<const uint8_t*>(key.data());
    size_t len = key.size();

    uint64_t h1 = 0x9368e53c2f6af274ULL;
    uint64_t h2 = 0x586dcd208f7cd3fdULL;

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

uint64_t fxHash(const std::string& key) {
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
    const uint64_t prime1 = 11400714785074694791ULL;
    const uint64_t prime2 = 14029467366897019727ULL;
    const uint64_t prime3 = 1609587929392839161ULL;
    const uint64_t prime4 = 9650029242287828579ULL;
    const uint64_t prime5 = 2870177450012600261ULL;

    const unsigned char* data =
        reinterpret_cast<const unsigned char*>(key.data());

    size_t len = key.size();

    uint64_t hash = prime5 + len;

    while (len >= 8) {
        uint64_t value = 0;

        std::memcpy(&value, data, 8);

        value *= prime2;
        value = (value << 31) | (value >> 33);
        value *= prime1;

        hash ^= value;
        hash = ((hash << 27) | (hash >> 37)) * prime1 + prime4;

        data += 8;
        len -= 8;
    }

    while (len >= 4) {
        uint32_t value = 0;

        std::memcpy(&value, data, 4);

        hash ^= static_cast<uint64_t>(value) * prime1;
        hash = ((hash << 23) | (hash >> 41)) * prime2 + prime3;

        data += 4;
        len -= 4;
    }

    while (len > 0) {
        hash ^= static_cast<uint64_t>(*data) * prime5;
        hash = ((hash << 11) | (hash >> 53)) * prime1;

        data++;
        len--;
    }

    hash ^= hash >> 33;
    hash *= prime2;
    hash ^= hash >> 29;
    hash *= prime3;
    hash ^= hash >> 32;

    return hash;
}

uint64_t getHash(const std::string& key, HashType type) {
    switch (type) {
        case HashType::MURMURHASH3:
            return murmurHash3(key);

        case HashType::FXHASH:
            return fxHash(key);

        case HashType::XXHASH:
            return xxHash(key);
    }

    return 0;
}

std::string hashTypeToString(HashType type) {
    switch (type) {
        case HashType::MURMURHASH3:
            return "MurmurHash3";

        case HashType::FXHASH:
            return "FxHash";

        case HashType::XXHASH:
            return "xxHash";
    }

    return "Unknown";
}