#ifndef HASH_FUNCTIONS_H
#define HASH_FUNCTIONS_H

#include <string>
#include <cstdint>

enum class HashType {
    MURMURHASH3,
    FXHASH,
    XXHASH
};

uint64_t murmurHash3(const std::string& key);
uint64_t fxHash(const std::string& key);
uint64_t xxHash(const std::string& key);

uint64_t getHash(const std::string& key, HashType type);

std::string hashTypeToString(HashType type);

#endif