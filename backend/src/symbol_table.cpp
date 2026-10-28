#include "../include/symbol_table.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <chrono>
#include <unordered_set>

namespace {
const char* const KEYWORDS[] = {
    "alignas","alignof","and","and_eq","asm","auto","bitand","bitor","bool","break",
    "case","catch","char","char8_t","char16_t","char32_t","class","compl","concept",
    "const","consteval","constexpr","constinit","const_cast","continue","co_await",
    "co_return","co_yield","decltype","default","delete","do","double","dynamic_cast",
    "else","enum","explicit","export","extern","false","float","for","friend","goto",
    "if","inline","int","long","mutable","namespace","new","noexcept","not","not_eq",
    "nullptr","operator","or","or_eq","private","protected","public","register",
    "reinterpret_cast","requires","return","short","signed","sizeof","static",
    "static_assert","static_cast","struct","switch","template","this","thread_local",
    "throw","true","try","typedef","typeid","typename","union","unsigned","using",
    "virtual","void","volatile","wchar_t","while","xor","xor_eq"
};
const size_t KEYWORD_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);

bool isReservedKeyword(const std::string& name) {
    for (size_t i = 0; i < KEYWORD_COUNT; ++i) {
        if (name == KEYWORDS[i]) return true;
    }
    return false;
}
}

Symbol::Symbol() : name(""), type(""), scope(""), line(0), occupied(false) {}
Symbol::Symbol(const std::string& n, const std::string& t,
               const std::string& s, int l)
    : name(n), type(t), scope(s), line(l), occupied(true) {}
TableEntry::TableEntry() : symbol(), hashValue(0), probeDistance(0), occupied(false) {}

SymbolTable::SymbolTable(size_t size, HashType h, CollisionMethod c, SymbolTableOptions opts)
    : capacity(size), elementCount(0), collisionCount(0), totalProbes(0),
      maxProbeDistance(0), lastSearchProbeCount(0), hashType(h), collisionMethod(c), robinHoodPhase(false),
      rehashCount(0), peakTableArrayBytes(size * sizeof(TableEntry)), rehashDurationMs(0.0), keywordHits(0), keywordSeed(0), options(opts) {
    if (capacity == 0) throw std::invalid_argument("SymbolTable capacity must be greater than zero");
    // Triangular-number probing is guaranteed to visit every slot for power-of-two
    // capacities. Round these strategies upward instead of silently accepting a
    // capacity at which an empty slot might be unreachable by the probe sequence.
    if (collisionMethod == CollisionMethod::QUADRATIC || collisionMethod == CollisionMethod::HYBRID) {
        size_t rounded = 1;
        while (rounded < capacity) {
            if (rounded > (static_cast<size_t>(-1) >> 1))
                throw std::length_error("Requested capacity is too large to round to a power of two");
            rounded <<= 1;
        }
        capacity = rounded;
    }
    peakTableArrayBytes = capacity * sizeof(TableEntry);
    table.resize(capacity);
}

size_t SymbolTable::primaryIndex(const std::string& key) const { return getHash(key, hashType) % capacity; }
size_t SymbolTable::secondaryHash(const std::string& key) const {
    if (capacity <= 1) return 1;
    // A full-cycle double-hash probe sequence requires gcd(step, capacity)=1.
    // This matters for power-of-two benchmark capacities: an arbitrary step
    // can otherwise visit only a subset of slots and fail despite free space.
    size_t step = 1 + static_cast<size_t>(getHash(key, HashType::XXHASH) % (capacity - 1));
    while (std::gcd(step, capacity) != 1) {
        ++step;
        if (step >= capacity) step = 1;
    }
    return step;
}

bool SymbolTable::insert(const Symbol& symbol) {
    if (options.rejectKeywords && isReservedKeyword(symbol.name)) { ++keywordHits; return false; }
    if (options.autoGrow && (static_cast<double>(elementCount + 1) / capacity) > options.maxLoadFactor) {
        size_t next = std::max<size_t>(2, capacity * 2);
        size_t pow2 = 1;
        while (pow2 < next && pow2 <= (static_cast<size_t>(-1) >> 1)) pow2 <<= 1;
        rehashTo(pow2);
    }
    Symbol existing;
    if (options.precheckDuplicates && search(symbol.name, existing)) return false;
    if (elementCount >= capacity) return false;
    if (collisionMethod == CollisionMethod::HYBRID) return insertHybrid(symbol);
    if (!options.precheckDuplicates && collisionMethod == CollisionMethod::ROBIN_HOOD) {
        Symbol existing;
        if (search(symbol.name, existing)) return false;
    }

    const uint64_t hashValue = getHash(symbol.name, hashType);
    if (collisionMethod == CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING) {
        TableEntry current;
        current.symbol = symbol; current.hashValue = hashValue;
        current.probeDistance = 0; current.occupied = true;
        return insertRobinHoodDoubleHashing(current);
    }
    if (collisionMethod == CollisionMethod::ROBIN_HOOD) {
        TableEntry current;
        current.symbol = symbol; current.hashValue = hashValue;
        current.probeDistance = 0; current.occupied = true;
        size_t index = static_cast<size_t>(hashValue % capacity);
        for (size_t probes = 0; probes < capacity; ++probes) {
            if (!table[index].occupied) {
                table[index] = current; ++elementCount;
                maxProbeDistance = std::max(maxProbeDistance, static_cast<size_t>(current.probeDistance));
                return true;
            }
            if (!options.unsafeAssumeUniqueKeysForBenchmark && table[index].symbol.name == symbol.name) return false;
            if (current.probeDistance > table[index].probeDistance) std::swap(current, table[index]);
            ++collisionCount; ++totalProbes;
            ++current.probeDistance;
            index = (index + 1) % capacity;
        }
        return false;
    }

    const size_t start = static_cast<size_t>(hashValue % capacity);
    const size_t step = secondaryHash(symbol.name);
    for (size_t i = 0; i < capacity; ++i) {
        size_t index = 0;
        if (collisionMethod == CollisionMethod::LINEAR) index = (start + i) % capacity;
        else if (collisionMethod == CollisionMethod::QUADRATIC) index = (start + (i * (i + 1) / 2)) % capacity;
        else index = (start + i * step) % capacity;

        if (!table[index].occupied) {
            table[index].symbol = symbol; table[index].hashValue = hashValue;
            table[index].probeDistance = static_cast<int>(i); table[index].occupied = true;
            ++elementCount; maxProbeDistance = std::max(maxProbeDistance, i); return true;
        }
        if (!options.unsafeAssumeUniqueKeysForBenchmark && table[index].symbol.name == symbol.name) return false;
        ++collisionCount; ++totalProbes;
    }
    return false;
}

bool SymbolTable::search(const std::string& name, Symbol& result) const {
    lastSearchProbeCount = 0;
    if (options.rejectKeywords && isReservedKeyword(name)) return false;
    if (collisionMethod == CollisionMethod::HYBRID) return searchHybrid(name, result);
    const size_t start = static_cast<size_t>(getHash(name, hashType) % capacity);
    if (collisionMethod == CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING) {
        const size_t step = secondaryHash(name);
        size_t index = start;
        // Different keys have different probe sequences, so no Robin Hood
        // early-miss rule is used. Stop only at an empty slot or after capacity.
        for (size_t i=0; i<capacity; ++i) {
            ++lastSearchProbeCount;
            if (!table[index].occupied) return false;
            if (table[index].symbol.name == name) { result=table[index].symbol; return true; }
            index=(index+step)%capacity;
        }
        return false;
    }
    const size_t step = secondaryHash(name);
    for (size_t i = 0; i < capacity; ++i) {
        ++lastSearchProbeCount;
        size_t index = 0;
        if (collisionMethod == CollisionMethod::LINEAR || collisionMethod == CollisionMethod::ROBIN_HOOD)
            index = (start + i) % capacity;
        else if (collisionMethod == CollisionMethod::QUADRATIC)
            index = (start + (i * (i + 1) / 2)) % capacity;
        else index = (start + i * step) % capacity;
        if (!table[index].occupied) return false;
        if (table[index].symbol.name == name) { result = table[index].symbol; return true; }
        // Robin Hood ordering permits an early miss when the resident's
        // displacement is smaller than the query's current displacement.
        if (collisionMethod == CollisionMethod::ROBIN_HOOD && table[index].probeDistance < static_cast<int>(i)) return false;
    }
    return false;
}

bool SymbolTable::erase(const std::string& name) {
    const auto rehashStart = std::chrono::steady_clock::now();
    peakTableArrayBytes = std::max(peakTableArrayBytes, 2 * capacity * sizeof(TableEntry));
    std::vector<Symbol> survivors;
    survivors.reserve(elementCount);
    bool found = false;
    for (const auto& entry : table) {
        if (!entry.occupied) continue;
        if (entry.symbol.name == name) found = true;
        else survivors.push_back(entry.symbol);
    }
    if (!found) return false;
    const size_t oldCapacity = capacity;
    // Rebuild avoids tombstone correctness hazards for quadratic/double-hash
    // probe sequences and preserves each strategy's lookup invariants.
    table.assign(capacity, TableEntry());
    elementCount = 0;
    maxProbeDistance = 0;
    robinHoodPhase = false;
    if (collisionMethod == CollisionMethod::HYBRID && options.adaptiveHybridSwitch &&
        !survivors.empty() && static_cast<double>(survivors.size()) / capacity >= HYBRID_SWITCH_LOAD)
        robinHoodPhase = true;
    for (const auto& symbol : survivors) {
        if (!insert(symbol)) throw std::logic_error("SymbolTable erase rebuild failed");
    }
    ++rehashCount;
    rehashDurationMs += std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now() - rehashStart).count();
    (void)oldCapacity;
    return true;
}

void SymbolTable::setAutoGrow(bool enabled) { options.autoGrow = enabled; }

void SymbolTable::rehashTo(size_t newCapacity) {
    if (newCapacity <= capacity) return;
    const auto rehashStart = std::chrono::steady_clock::now();
    peakTableArrayBytes = std::max(peakTableArrayBytes,
        capacity * sizeof(TableEntry) + newCapacity * sizeof(TableEntry));
    std::vector<Symbol> survivors;
    survivors.reserve(elementCount);
    for (const auto& entry : table) if (entry.occupied) survivors.push_back(entry.symbol);
    capacity = newCapacity;
    table.assign(capacity, TableEntry());
    elementCount = 0;
    maxProbeDistance = 0;
    robinHoodPhase = false;
    ++rehashCount;
    for (const auto& symbol : survivors) {
        if (!insert(symbol)) throw std::logic_error("SymbolTable growth rehash failed");
    }
    rehashDurationMs += std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now() - rehashStart).count();
}

void SymbolTable::clear() {
    for (auto& entry : table) entry = TableEntry();
    elementCount = collisionCount = totalProbes = maxProbeDistance = 0;
    robinHoodPhase = false; rehashCount = keywordHits = 0;
    peakTableArrayBytes = capacity * sizeof(TableEntry);
    rehashDurationMs = 0.0;
}
size_t SymbolTable::getCollisionCount() const { return collisionCount; }
size_t SymbolTable::getTotalProbes() const { return totalProbes; }
size_t SymbolTable::getMaxProbeDistance() const { return maxProbeDistance; }
size_t SymbolTable::getElementCount() const { return elementCount; }
size_t SymbolTable::getRehashCount() const { return rehashCount; }
size_t SymbolTable::getPeakTableArrayBytes() const { return peakTableArrayBytes; }
double SymbolTable::getRehashDurationMs() const { return rehashDurationMs; }
size_t SymbolTable::getKeywordHits() const { return keywordHits; }
size_t SymbolTable::getLastSearchProbeCount() const { return lastSearchProbeCount; }
double SymbolTable::getAverageProbes() const {
    if (elementCount == 0) return 0.0;
    size_t probeSum = 0;
    for (const auto& entry : table) {
        if (entry.occupied) probeSum += static_cast<size_t>(entry.probeDistance) + 1;
    }
    return static_cast<double>(probeSum) / static_cast<double>(elementCount);
}
double SymbolTable::getLoadFactor() const { return static_cast<double>(elementCount) / capacity; }
const std::vector<TableEntry>& SymbolTable::getTable() const { return table; }
std::string SymbolTable::getHashName() const { return hashTypeToString(hashType); }
std::string SymbolTable::getCollisionMethodName() const { return collisionMethodToString(collisionMethod); }

ScopedSymbolEnvironment::ScopedSymbolEnvironment(size_t initialCapacity, HashType hashType,
        CollisionMethod method, SymbolTableOptions options)
    : hashType_(hashType), method_(method), initialCapacity_(initialCapacity), options_(options) {
    if (initialCapacity_ == 0) throw std::invalid_argument("Scope table capacity must be positive");
    enterScope("global");
}
void ScopedSymbolEnvironment::enterScope(const std::string& name) {
    ScopeFrame frame;
    frame.name = name;
    frame.table = std::make_unique<SymbolTable>(initialCapacity_, hashType_, method_, options_);
    scopes_.push_back(std::move(frame));
}
bool ScopedSymbolEnvironment::exitScope() {
    if (scopes_.size() <= 1) return false; // global scope cannot be removed
    historicalRehashes_ += scopes_.back().table->getRehashCount();
    scopes_.pop_back();
    return true;
}
bool ScopedSymbolEnvironment::insert(const Symbol& symbol) {
    if (scopes_.empty()) enterScope("global");
    Symbol scoped = symbol;
    scoped.scope = scopes_.back().name;
    return scopes_.back().table->insert(scoped);
}
bool ScopedSymbolEnvironment::search(const std::string& name, Symbol& result) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it)
        if (it->table->search(name, result)) return true;
    return false;
}
bool ScopedSymbolEnvironment::erase(const std::string& name) {
    if (scopes_.empty()) return false;
    return scopes_.back().table->erase(name);
}
size_t ScopedSymbolEnvironment::scopeDepth() const { return scopes_.size(); }
size_t ScopedSymbolEnvironment::getTotalRehashCount() const {
    size_t total=historicalRehashes_;
    for(const auto& scope:scopes_) total += scope.table->getRehashCount();
    return total;
}

std::string collisionMethodToString(CollisionMethod method) {
    switch (method) {
        case CollisionMethod::LINEAR: return "Linear Probing";
        case CollisionMethod::QUADRATIC: return "Quadratic Probing";
        case CollisionMethod::DOUBLE_HASHING: return "Double Hashing";
        case CollisionMethod::ROBIN_HOOD: return "Robin Hood (Linear Probing)";
        case CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING: return "Robin Hood Double Hashing (full-scan misses)";
        case CollisionMethod::HYBRID: return "Adaptive Quadratic-to-Robin-Hood";
    }
    return "Unknown";
}

// Keyword membership is deliberately handled consistently by insert/search
// for every collision strategy. The fixed keyword list is a semantic guard,
// not an unmeasured shortcut that makes the hybrid method incomparable.
void SymbolTable::buildKeywordTable() {}
bool SymbolTable::isKeyword(const std::string& name) const { return isReservedKeyword(name); }

bool SymbolTable::insertHybrid(const Symbol& symbol) {
    // A duplicate is rejected before any relocation or phase transition.
    Symbol existing;
    if (options.precheckDuplicates && searchHybrid(symbol.name, existing)) return false;
    if (options.adaptiveHybridSwitch && !robinHoodPhase && getLoadFactor() >= HYBRID_SWITCH_LOAD) switchToRobinHood();

    if (robinHoodPhase) {
        TableEntry entry; entry.symbol = symbol; entry.hashValue = getHash(symbol.name, hashType);
        entry.probeDistance = 0; entry.occupied = true;
        return insertRobinHood(entry);
    }
    const uint64_t hashValue = getHash(symbol.name, hashType);
    const size_t start = static_cast<size_t>(hashValue % capacity);
    for (size_t i = 0; i < capacity; ++i) {
        const size_t index = (start + (i * (i + 1) / 2)) % capacity;
        if (!table[index].occupied) {
            table[index].symbol = symbol; table[index].hashValue = hashValue;
            table[index].probeDistance = static_cast<int>(i); table[index].occupied = true;
            ++elementCount; maxProbeDistance = std::max(maxProbeDistance, i); return true;
        }
        if (!options.unsafeAssumeUniqueKeysForBenchmark && table[index].symbol.name == symbol.name) return false;
        ++collisionCount; ++totalProbes;
    }
    return false;
}

bool SymbolTable::insertRobinHood(TableEntry current) {
    // When the redundant outer duplicate precheck is disabled for an ablation,
    // still verify the key before any swaps so a duplicate can never corrupt
    // the table. This guard is part of the algorithm's correctness.
    if (!options.precheckDuplicates && !options.unsafeAssumeUniqueKeysForBenchmark) {
        size_t probeIndex = static_cast<size_t>(current.hashValue % capacity);
        for (size_t i=0; i<capacity; ++i) {
            if (!table[probeIndex].occupied) break;
            if (!options.unsafeAssumeUniqueKeysForBenchmark && table[probeIndex].symbol.name == current.symbol.name) return false;
            if (table[probeIndex].probeDistance < static_cast<int>(i)) break;
            probeIndex = (probeIndex + 1) % capacity;
        }
    }
    // This is classical Robin Hood hashing with linear probing. Applying
    // Robin Hood swaps to key-specific double-hash sequences is not valid.
    size_t index = static_cast<size_t>(current.hashValue % capacity);
    for (size_t probes = 0; probes < capacity; ++probes) {
        if (!table[index].occupied) {
            table[index] = current; ++elementCount;
            maxProbeDistance = std::max(maxProbeDistance, static_cast<size_t>(current.probeDistance));
            return true;
        }
        if (!options.unsafeAssumeUniqueKeysForBenchmark && table[index].symbol.name == current.symbol.name) return false;
        if (current.probeDistance > table[index].probeDistance) std::swap(current, table[index]);
        ++collisionCount; ++totalProbes; ++current.probeDistance;
        index = (index + 1) % capacity;
    }
    return false;
}

bool SymbolTable::insertRobinHoodDoubleHashing(TableEntry current) {
    // Robin Hood insertion over key-specific double-hash sequences. A displaced
    // resident continues along its own secondary step. Searches deliberately
    // do not use the linear-Robin-Hood early-termination rule.
    if (!options.precheckDuplicates && !options.unsafeAssumeUniqueKeysForBenchmark) {
        Symbol existing;
        if (search(current.symbol.name, existing)) return false;
    }
    size_t index=static_cast<size_t>(current.hashValue%capacity);
    for(size_t probes=0; probes<capacity; ++probes) {
        if(!table[index].occupied) {
            table[index]=current; ++elementCount;
            maxProbeDistance=std::max(maxProbeDistance,static_cast<size_t>(current.probeDistance));
            return true;
        }
        if(table[index].symbol.name==current.symbol.name) return false;
        if(current.probeDistance>table[index].probeDistance) std::swap(current,table[index]);
        ++collisionCount; ++totalProbes;
        ++current.probeDistance;
        const size_t step=secondaryHash(current.symbol.name);
        index=(index+step)%capacity;
    }
    return false;
}

void SymbolTable::switchToRobinHood() {
    const auto rehashStart = std::chrono::steady_clock::now();
    peakTableArrayBytes = std::max(peakTableArrayBytes, 2 * capacity * sizeof(TableEntry));
    std::vector<TableEntry> old = table;
    table.assign(capacity, TableEntry()); elementCount = 0; maxProbeDistance = 0;
    robinHoodPhase = true; ++rehashCount;
    for (const auto& oldEntry : old) {
        if (!oldEntry.occupied) continue;
        TableEntry entry = oldEntry; entry.probeDistance = 0;
        // Rehash metrics are kept separate from normal insert metrics only in
        // documentation; the counters include the work required to rehash.
        insertRobinHood(entry);
    }
    rehashDurationMs += std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now() - rehashStart).count();
}

bool SymbolTable::searchHybrid(const std::string& name, Symbol& result) const {
    lastSearchProbeCount = 0;
    const size_t start = static_cast<size_t>(getHash(name, hashType) % capacity);
    for (size_t i = 0; i < capacity; ++i) {
        ++lastSearchProbeCount;
        const size_t index = robinHoodPhase ? (start + i) % capacity
                                            : (start + (i * (i + 1) / 2)) % capacity;
        if (!table[index].occupied) return false;
        if (table[index].symbol.name == name) { result = table[index].symbol; return true; }
        if (robinHoodPhase && table[index].probeDistance < static_cast<int>(i)) return false;
    }
    return false;
}
