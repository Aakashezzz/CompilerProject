#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <string>
#include <cstdint>

enum class HashType {
    MURMURHASH3,
    FNV1A_AVALANCHE,
    XXHASH
};

uint64_t murmurHash3(const std::string& key);
uint64_t fnv1aAvalancheHash(const std::string& key);
uint64_t xxHash(const std::string& key);

uint64_t getHash(const std::string& key, HashType type);

std::string hashTypeToString(HashType type);

#endif