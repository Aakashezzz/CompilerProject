#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "hash_functions.h"
#include <string>
#include <vector>

enum class CollisionMethod {
    LINEAR,
    QUADRATIC,
    DOUBLE_HASHING,
    ROBIN_HOOD
};

struct Symbol {
    std::string name;
    std::string type;
    std::string scope;
    int line;
    bool occupied;

    Symbol();
    Symbol(const std::string& n, const std::string& t,
           const std::string& s, int l);
};

struct TableEntry {
    Symbol symbol;
    uint64_t hashValue;
    int probeDistance;
    bool occupied;

    TableEntry();
};

class SymbolTable {
private:
    std::vector<TableEntry> table;
    size_t capacity;
    size_t elementCount;
    size_t collisionCount;
    size_t totalProbes;
    size_t maxProbeDistance;

    HashType hashType;
    CollisionMethod collisionMethod;

    size_t primaryIndex(const std::string& key) const;
    size_t secondaryHash(const std::string& key) const;

public:
    SymbolTable(size_t size, HashType hashType,
                CollisionMethod collisionMethod);

    bool insert(const Symbol& symbol);
    bool search(const std::string& name, Symbol& result) const;

    void clear();

    size_t getCollisionCount() const;
    size_t getTotalProbes() const;
    size_t getMaxProbeDistance() const;
    size_t getElementCount() const;

    double getAverageProbes() const;
    double getLoadFactor() const;

    const std::vector<TableEntry>& getTable() const;

    std::string getHashName() const;
    std::string getCollisionMethodName() const;
};

std::string collisionMethodToString(CollisionMethod method);

#endif