#include <stdexcept>
#include "../include/symbol_table.h"
#include <cassert>
#include <algorithm>
#include <random>
#include <iostream>
#include <string>
#include <vector>

static void testStrategy(CollisionMethod method) {
    SymbolTable table(128, HashType::MURMURHASH3, method);
    std::vector<Symbol> symbols;
    for (int i=0;i<70;++i) symbols.emplace_back("identifier_"+std::to_string(i),"int",i%2?"local":"global",i+1);
    for (const auto& s:symbols) assert(table.insert(s));
    assert(table.getElementCount()==symbols.size());
    for (const auto& s:symbols) { Symbol found; assert(table.search(s.name,found)); assert(found.type==s.type); assert(found.scope==s.scope); }
    // Duplicate identifiers are rejected and must not mutate the table.
    assert(!table.insert(symbols[10])); assert(table.getElementCount()==symbols.size());
    Symbol missing; assert(!table.search("definitely_missing",missing));
    // Reserved words are consistently rejected, not silently treated as stored symbols.
    assert(!table.insert(Symbol("while","keyword","global",0)));
    assert(!table.search("while",missing));
    assert(table.erase("identifier_20"));
    assert(!table.search("identifier_20",missing));
    assert(table.getElementCount()==symbols.size()-1);
    for (const auto& s : symbols) {
        Symbol found;
        if (s.name=="identifier_20") assert(!table.search(s.name,found));
        else assert(table.search(s.name,found));
    }
}
static void testHashVectors() {
    // Published XXH64 seed-0 vectors: empty string and "a".
    assert(xxHash("") == 0xef46db3751d8e999ULL);
    assert(xxHash("a") == 0xd24ec4f1a98c6e5bULL);
    // MurmurHash3 x64/128 with seed 0 returns zero low word for empty input.
    assert(murmurHash3("") == 0ULL);
    assert(fnv1aAvalancheHash("") != fnv1aAvalancheHash("a"));
}

static void testRandomizedRoundTrips(CollisionMethod method) {
    // Exercise high-load inserts, searches, deletion/rebuild, and post-erase lookups.
    for (unsigned seed=0; seed<12; ++seed) {
        SymbolTableOptions opts; opts.autoGrow = true; opts.maxLoadFactor = 0.72;
        SymbolTable table(32, HashType::XXHASH, method, opts);
        std::vector<std::string> keys;
        for (int i=0; i<180; ++i) keys.push_back("stress_"+std::to_string(seed)+"_"+std::to_string(i));
        std::mt19937 rng(seed+2026); std::shuffle(keys.begin(), keys.end(), rng);
        for (const auto& key : keys) assert(table.insert(Symbol(key,"int","stress",1)));
        for (const auto& key : keys) { Symbol found; assert(table.search(key,found)); assert(found.name==key); }
        for (size_t i=0; i<keys.size(); i+=3) assert(table.erase(keys[i]));
        for (size_t i=0; i<keys.size(); ++i) {
            Symbol found;
            if (i%3==0) assert(!table.search(keys[i],found));
            else assert(table.search(keys[i],found));
        }
    }
}

int main() {
    testHashVectors();
    for (CollisionMethod m : {CollisionMethod::LINEAR,CollisionMethod::QUADRATIC,
                              CollisionMethod::DOUBLE_HASHING,CollisionMethod::ROBIN_HOOD,
                              CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING,CollisionMethod::HYBRID}) {
        testStrategy(m);
        testRandomizedRoundTrips(m);
    }
    SymbolTable small(1,HashType::FNV1A_AVALANCHE,CollisionMethod::LINEAR);
    assert(small.insert(Symbol("only","int","global",1)));
    Symbol found; assert(small.search("only",found));
    bool threw=false; try { SymbolTable invalid(0,HashType::FNV1A_AVALANCHE,CollisionMethod::LINEAR); }
    catch(const std::invalid_argument&) { threw=true; }
    assert(threw);

    SymbolTableOptions growOptions;
    growOptions.autoGrow = true;
    growOptions.maxLoadFactor = 0.60;
    SymbolTable growing(4, HashType::XXHASH, CollisionMethod::QUADRATIC, growOptions);
    for (int i=0;i<80;++i) assert(growing.insert(Symbol("grow_"+std::to_string(i),"int","global",i)));
    for (int i=0;i<80;++i) { Symbol x; assert(growing.search("grow_"+std::to_string(i),x)); }
    assert(growing.getRehashCount() > 0);
    assert(growing.erase("grow_10"));
    { Symbol x; assert(!growing.search("grow_10",x)); }
    for (int i=0;i<80;++i) if(i!=10) { Symbol x; assert(growing.search("grow_"+std::to_string(i),x)); }

    SymbolTableOptions noPrecheckOptions;
    noPrecheckOptions.precheckDuplicates = false;
    SymbolTable noPrecheckDoubleRH(64, HashType::XXHASH, CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING, noPrecheckOptions);
    for(int i=0;i<40;++i) assert(noPrecheckDoubleRH.insert(Symbol("dp_"+std::to_string(i),"int","global",i)));
    assert(!noPrecheckDoubleRH.insert(Symbol("dp_5","int","global",99)));
    for(int i=0;i<40;++i) { Symbol x; assert(noPrecheckDoubleRH.search("dp_"+std::to_string(i),x)); }

    SymbolTable noPrecheckHybrid(64, HashType::XXHASH, CollisionMethod::HYBRID, noPrecheckOptions);
    for(int i=0;i<40;++i) assert(noPrecheckHybrid.insert(Symbol("np_"+std::to_string(i),"int","global",i)));
    assert(!noPrecheckHybrid.insert(Symbol("np_5","int","global",99)));
    for(int i=0;i<40;++i) { Symbol x; assert(noPrecheckHybrid.search("np_"+std::to_string(i),x)); }

    ScopedSymbolEnvironment env(32, HashType::MURMURHASH3, CollisionMethod::ROBIN_HOOD);
    assert(env.insert(Symbol("value","int","ignored",1)));
    env.enterScope("function");
    assert(env.insert(Symbol("value","float","ignored",2)));
    Symbol scoped;
    assert(env.search("value",scoped) && scoped.type=="float");
    assert(env.exitScope());
    assert(env.search("value",scoped) && scoped.type=="int");
    assert(!env.exitScope());
    std::cout << "All symbol table tests passed.\\n";
}
