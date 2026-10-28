#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "hash_functions.h"
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

enum class CollisionMethod {
    LINEAR,
    QUADRATIC,
    DOUBLE_HASHING,
    ROBIN_HOOD,
    ROBIN_HOOD_DOUBLE_HASHING,
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

struct SymbolTableOptions {
    bool rejectKeywords = true;
    bool precheckDuplicates = true;
    // Benchmark-only switch: caller guarantees all keys are unique. Never enable for general symbol-table use.
    bool unsafeAssumeUniqueKeysForBenchmark = false;
    bool adaptiveHybridSwitch = true;
    bool autoGrow = false;
    double maxLoadFactor = 0.70;
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
    mutable size_t lastSearchProbeCount;

    HashType hashType;
    CollisionMethod collisionMethod;
        // --- hybrid method state ---
    bool robinHoodPhase;
    size_t rehashCount;
    size_t peakTableArrayBytes;
    double rehashDurationMs;
    size_t keywordHits;
    uint64_t keywordSeed;
    std::vector<int> keywordSlots;
    SymbolTableOptions options;

    static constexpr double HYBRID_SWITCH_LOAD = 0.5;

    bool insertHybrid(const Symbol& symbol);
    bool searchHybrid(const std::string& name, Symbol& result) const;
    bool insertRobinHood(TableEntry current);
    bool insertRobinHoodDoubleHashing(TableEntry current);
    void switchToRobinHood();
    void buildKeywordTable();
    bool isKeyword(const std::string& name) const;
    void rehashTo(size_t newCapacity);

    size_t primaryIndex(const std::string& key) const;
    size_t secondaryHash(const std::string& key) const;

public:
    SymbolTable(size_t size, HashType hashType,
                CollisionMethod collisionMethod, SymbolTableOptions options = {});

    bool insert(const Symbol& symbol);
    bool search(const std::string& name, Symbol& result) const;
    bool erase(const std::string& name);
    void setAutoGrow(bool enabled);

    void clear();

    size_t getCollisionCount() const;
    size_t getTotalProbes() const;
    size_t getMaxProbeDistance() const;
    size_t getElementCount() const;
        size_t getRehashCount() const;
    size_t getPeakTableArrayBytes() const;
    double getRehashDurationMs() const;
    size_t getKeywordHits() const;
    size_t getLastSearchProbeCount() const;

    double getAverageProbes() const;
    double getLoadFactor() const;

    const std::vector<TableEntry>& getTable() const;

    std::string getHashName() const;
    std::string getCollisionMethodName() const;
};

// A small scope-aware environment wrapper. Each scope owns a table; lookup
// searches from the innermost scope outwards, and exitScope discards locals.
class ScopedSymbolEnvironment {
private:
    struct ScopeFrame {
        std::string name;
        std::unique_ptr<SymbolTable> table;
    };
    HashType hashType_;
    CollisionMethod method_;
    size_t initialCapacity_;
    SymbolTableOptions options_;
    std::vector<ScopeFrame> scopes_;
    size_t historicalRehashes_ = 0;
public:
    ScopedSymbolEnvironment(size_t initialCapacity, HashType hashType,
                           CollisionMethod method, SymbolTableOptions options = {});
    void enterScope(const std::string& name);
    bool exitScope();
    bool insert(const Symbol& symbol);
    bool search(const std::string& name, Symbol& result) const;
    bool erase(const std::string& name);
    size_t scopeDepth() const;
    size_t getTotalRehashCount() const;
};

std::string collisionMethodToString(CollisionMethod method);

#endif