#include "../include/symbol_table.h"
#include <functional>

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
      collisionMethod(c) {
    table.resize(capacity);
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

                totalProbes += current.probeDistance;
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
            totalProbes += i;

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
    }

    return "Unknown";
}