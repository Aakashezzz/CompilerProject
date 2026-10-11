#include "../include/symbol_table.h"
#include <functional>
#include <algorithm>
#include <cstdint>

namespace {
const char* const KEYWORDS[] = {
    "auto","break","case","char","const","continue","default","do",
    "double","else","enum","extern","float","for","goto","if","int",
    "long","register","return","short","signed","sizeof","static",
    "struct","switch","typedef","union","unsigned","void","volatile",
    "while","bool","class","new","delete","true","false"
};
const size_t KEYWORD_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);
const size_t KEYWORD_SLOTS = 256;

uint64_t keywordHash(const std::string& s, uint64_t seed) {
    uint64_t h = 14695981039346656037ULL ^ seed;   // FNV-1a with a seed
    for (unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h ^ (h >> 32);
}
}

Symbol::Symbol()
    : name(""), type(""), scope(""), line(0), occupied(false) {}

Symbol::Symbol(const std::string& n, const std::string& t,
               const std::string& s, int l)
    : name(n), type(t), scope(s), line(l), occupied(true) {}

TableEntry::TableEntry()
    : symbol(), hashValue(0), probeDistance(0), occupied(false) {}

SymbolTable::SymbolTable(size_t size, HashType h,
                         CollisionMethod c)
    : capacity(size),
      elementCount(0),
      collisionCount(0),
      totalProbes(0),
      maxProbeDistance(0),
      hashType(h),
      collisionMethod(c),
      robinHoodPhase(false),
      rehashCount(0),
      keywordHits(0),
      keywordSeed(0) {
    table.resize(capacity);
    if (collisionMethod == CollisionMethod::HYBRID) {
        buildKeywordTable();
    }
}

size_t SymbolTable::primaryIndex(const std::string& key) const {
    return getHash(key, hashType) % capacity;
}

size_t SymbolTable::secondaryHash(const std::string& key) const {
    uint64_t hash = std::hash<std::string>{}(key);
    return 1 + (hash % (capacity - 1));
}

bool SymbolTable::insert(const Symbol& symbol) {
    if (elementCount >= capacity) {
        return false;
    }
    if (collisionMethod == CollisionMethod::HYBRID) {
        return insertHybrid(symbol);
    }

    if (collisionMethod == CollisionMethod::ROBIN_HOOD) {
        TableEntry current;
        current.symbol = symbol;
        current.hashValue = getHash(symbol.name, hashType);
        current.probeDistance = 0;
        current.occupied = true;

        size_t index = current.hashValue % capacity;

        for (size_t i = 0; i < capacity; i++) {
            if (!table[index].occupied) {
                table[index] = current;
                elementCount++;

                maxProbeDistance =
                    std::max(maxProbeDistance,
                             static_cast<size_t>(current.probeDistance));

                return true;
            }

            if (table[index].symbol.name == symbol.name) {
                return false;
            }

            if (current.probeDistance > table[index].probeDistance) {
                std::swap(current, table[index]);
            }

            collisionCount++;
            totalProbes++;

            current.probeDistance++;
            index = (index + 1) % capacity;
        }

        return false;
    }

    uint64_t hashValue = getHash(symbol.name, hashType);
    size_t start = hashValue % capacity;

    for (size_t i = 0; i < capacity; i++) {
        size_t index;

        if (collisionMethod == CollisionMethod::LINEAR) {
            index = (start + i) % capacity;
        }
        else if (collisionMethod == CollisionMethod::QUADRATIC) {
            index = (start + i * i) % capacity;
        }
        else {
            index = (start + i * secondaryHash(symbol.name)) % capacity;
        }

        if (!table[index].occupied) {
            table[index].symbol = symbol;
            table[index].hashValue = hashValue;
            table[index].probeDistance = static_cast<int>(i);
            table[index].occupied = true;

            elementCount++;
            

            maxProbeDistance =
                std::max(maxProbeDistance, i);

            return true;
        }

        if (table[index].symbol.name == symbol.name) {
            return false;
        }

        collisionCount++;
        totalProbes++;
    }

    return false;
}

bool SymbolTable::search(const std::string& name, Symbol& result) const {
        if (collisionMethod == CollisionMethod::HYBRID) {
        return searchHybrid(name, result);
    }
    uint64_t hashValue = getHash(name, hashType);
    size_t start = hashValue % capacity;

    for (size_t i = 0; i < capacity; i++) {
        size_t index;

        if (collisionMethod == CollisionMethod::LINEAR ||
            collisionMethod == CollisionMethod::ROBIN_HOOD) {
            index = (start + i) % capacity;
        }
        else if (collisionMethod == CollisionMethod::QUADRATIC) {
            index = (start + i * i) % capacity;
        }
        else {
            index = (start + i * secondaryHash(name)) % capacity;
        }

        if (!table[index].occupied) {
            return false;
        }

        if (table[index].symbol.name == name) {
            result = table[index].symbol;
            return true;
        }
    }

    return false;
}

void SymbolTable::clear() {
    for (auto& entry : table) {
        entry = TableEntry();
    }

    elementCount = 0;
    collisionCount = 0;
    totalProbes = 0;
    maxProbeDistance = 0;
    robinHoodPhase = false;
    rehashCount = 0;
    keywordHits = 0;
}

size_t SymbolTable::getCollisionCount() const {
    return collisionCount;
}

size_t SymbolTable::getTotalProbes() const {
    return totalProbes;
}

size_t SymbolTable::getMaxProbeDistance() const {
    return maxProbeDistance;
}

size_t SymbolTable::getElementCount() const {
    return elementCount;
}

double SymbolTable::getAverageProbes() const {
    if (elementCount == 0) {
        return 0.0;
    }

    return static_cast<double>(totalProbes) /
           static_cast<double>(elementCount);
}

double SymbolTable::getLoadFactor() const {
    return static_cast<double>(elementCount) /
           static_cast<double>(capacity);
}

const std::vector<TableEntry>& SymbolTable::getTable() const {
    return table;
}

std::string SymbolTable::getHashName() const {
    return hashTypeToString(hashType);
}

std::string SymbolTable::getCollisionMethodName() const {
    return collisionMethodToString(collisionMethod);
}

std::string collisionMethodToString(CollisionMethod method) {
    switch (method) {
        case CollisionMethod::LINEAR:
            return "Linear Probing";

        case CollisionMethod::QUADRATIC:
            return "Quadratic Probing";

        case CollisionMethod::DOUBLE_HASHING:
            return "Double Hashing";

        case CollisionMethod::ROBIN_HOOD:
            return "Robin Hood Hashing";

        case CollisionMethod::HYBRID:
            return "Adaptive Robin Hood Double Hashing";
    }

    return "Unknown";
}
size_t SymbolTable::getRehashCount() const { return rehashCount; }
size_t SymbolTable::getKeywordHits() const { return keywordHits; }

// ---------- keyword pre-hash ----------
void SymbolTable::buildKeywordTable() {
    // Try seeds until all keywords land in different slots (perfect hash).
    for (uint64_t seed = 1; seed < 100000; seed++) {
        std::vector<int> slots(KEYWORD_SLOTS, -1);
        bool ok = true;
        for (size_t k = 0; k < KEYWORD_COUNT && ok; k++) {
            size_t s = keywordHash(KEYWORDS[k], seed) % KEYWORD_SLOTS;
            if (slots[s] != -1) ok = false;
            else slots[s] = static_cast<int>(k);
        }
        if (ok) {
            keywordSlots = slots;
            keywordSeed = seed;
            return;
        }
    }
}

bool SymbolTable::isKeyword(const std::string& name) const {
    if (keywordSlots.empty()) return false;
    int k = keywordSlots[keywordHash(name, keywordSeed) % KEYWORD_SLOTS];
    return k != -1 && name == KEYWORDS[k];
}

// ---------- hybrid insert ----------
bool SymbolTable::insertHybrid(const Symbol& symbol) {
    // 1. Keywords never enter the main table.
    if (isKeyword(symbol.name)) {
        keywordHits++;
        return true;
    }

    // 2. Duplicate check up front (Robin Hood swaps can otherwise
    //    create duplicates when the probe step differs per key).
    Symbol existing;
    if (searchHybrid(symbol.name, existing)) {
        return false;
    }

    // 3. Adaptive switch: rehash once when load reaches the threshold.
    if (!robinHoodPhase && getLoadFactor() >= HYBRID_SWITCH_LOAD) {
        switchToRobinHoodDouble();
    }

    uint64_t hashValue = getHash(symbol.name, hashType);

    // Phase 2: Robin Hood + double hashing
    if (robinHoodPhase) {
        TableEntry entry;
        entry.symbol = symbol;
        entry.hashValue = hashValue;
        entry.probeDistance = 0;
        entry.occupied = true;
        return insertRobinHoodDouble(entry);
    }

    // Phase 1: quadratic probing
    size_t start = hashValue % capacity;
    for (size_t i = 0; i < capacity; i++) {
        size_t index = (start + i * i) % capacity;

        if (!table[index].occupied) {
            table[index].symbol = symbol;
            table[index].hashValue = hashValue;
            table[index].probeDistance = static_cast<int>(i);
            table[index].occupied = true;
            elementCount++;
            maxProbeDistance = std::max(maxProbeDistance, i);
            return true;
        }

        collisionCount++;
        totalProbes++;
    }
    return false;
}

bool SymbolTable::insertRobinHoodDouble(TableEntry current) {
    size_t index = current.hashValue % capacity;

    for (size_t n = 0; n < capacity * 2; n++) {
        if (!table[index].occupied) {
            table[index] = current;
            elementCount++;
            maxProbeDistance = std::max(
                maxProbeDistance, static_cast<size_t>(current.probeDistance));
            return true;
        }

        // Robin Hood rule: the key that has probed farther takes the slot.
        if (current.probeDistance > table[index].probeDistance) {
            std::swap(current, table[index]);
            maxProbeDistance = std::max(
                maxProbeDistance,
                static_cast<size_t>(table[index].probeDistance));
        }

        collisionCount++;
        totalProbes++;

        // 'current' may now be a displaced key: advance with ITS OWN step.
        current.probeDistance++;
        index = (index + secondaryHash(current.symbol.name)) % capacity;
    }
    return false;
}

void SymbolTable::switchToRobinHoodDouble() {
    std::vector<TableEntry> old = table;
    table.assign(capacity, TableEntry());
    elementCount = 0;
    maxProbeDistance = 0;      // re-measured on the new layout
    robinHoodPhase = true;
    rehashCount++;

    for (auto& e : old) {
        if (e.occupied) {
            e.probeDistance = 0;
            insertRobinHoodDouble(e);
        }
    }
}

// ---------- hybrid search ----------
bool SymbolTable::searchHybrid(const std::string& name, Symbol& result) const {
    if (isKeyword(name)) {
        result = Symbol(name, "keyword", "global", 0);
        return true;
    }

    size_t start = getHash(name, hashType) % capacity;
    size_t step = robinHoodPhase ? secondaryHash(name) : 0;

    for (size_t i = 0; i < capacity; i++) {
        size_t index = robinHoodPhase
                           ? (start + i * step) % capacity
                           : (start + i * i) % capacity;

        if (!table[index].occupied) return false;

        if (table[index].symbol.name == name) {
            result = table[index].symbol;
            return true;
        }
    }
    return false;
}