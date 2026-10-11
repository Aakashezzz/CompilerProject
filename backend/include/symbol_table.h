#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "hash_functions.h"
#include <string>
#include <vector>
#include <cstdint>

enum class CollisionMethod {
    LINEAR,
    QUADRATIC,
    DOUBLE_HASHING,
    ROBIN_HOOD,
    HYBRID
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
        // --- hybrid method state ---
    bool robinHoodPhase;
    size_t rehashCount;
    size_t keywordHits;
    uint64_t keywordSeed;
    std::vector<int> keywordSlots;

    static constexpr double HYBRID_SWITCH_LOAD = 0.5;

    bool insertHybrid(const Symbol& symbol);
    bool searchHybrid(const std::string& name, Symbol& result) const;
    bool insertRobinHoodDouble(TableEntry current);
    void switchToRobinHoodDouble();
    void buildKeywordTable();
    bool isKeyword(const std::string& name) const;

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
        size_t getRehashCount() const;
    size_t getKeywordHits() const;

    double getAverageProbes() const;
    double getLoadFactor() const;

    const std::vector<TableEntry>& getTable() const;

    std::string getHashName() const;
    std::string getCollisionMethodName() const;
};

std::string collisionMethodToString(CollisionMethod method);

#endif