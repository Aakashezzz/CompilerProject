#include "include/hash_functions.h"
#include "include/symbol_table.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <vector>
#if __has_include(<boost/unordered/unordered_flat_map.hpp>)
#include <boost/unordered/unordered_flat_map.hpp>
#define HAS_BOOST_FLAT_MAP 1
#else
#define HAS_BOOST_FLAT_MAP 0
#endif

using Clock = std::chrono::steady_clock;
namespace fs = std::filesystem;

struct Timings { double insertMs=0, successfulMs=0, unsuccessfulMs=0; };
struct Summary { double median=0, ci95=0; };
struct Result {
    std::string hash, method;
    double targetLoad=0, actualLoad=0;
    double insertionMedian=0, insertionCI=0, successfulMedian=0, successfulCI=0;
    double unsuccessfulMedian=0, unsuccessfulCI=0;
    double unsuccessfulMeanProbes=0, unsuccessfulP50=0, unsuccessfulP95=0, unsuccessfulMax=0;
    double collisions=0, totalProbes=0, averageProbes=0, maxProbeDistance=0, rehashDurationMs=0;
    int repetitions=0; size_t inserted=0, failedInserts=0, failedSearches=0, rehashCount=0, tableArrayBytes=0, peakTableArrayBytes=0, bytesPerSlot=0;
};

static std::vector<Symbol> makeWorkload(size_t count, uint32_t seed) {
    // Synthetic compiler-like identifiers: varied lengths, prefixes, types,
    // source lines and scope labels; the generator is deterministic per seed.
    std::mt19937 rng(seed);
    const std::vector<std::string> prefixes={"tmp","node","token","symbol","value","result","counter","buffer","state","ast"};
    const std::vector<std::string> types={"int","float","char","bool","Node*","std::string"};
    const std::vector<std::string> scopes={"global","function_main","function_parse","block_1","block_2","class_Parser"};
    std::vector<Symbol> out; out.reserve(count);
    for(size_t i=0;i<count;++i){
        const auto p=prefixes[rng()%prefixes.size()];
        const auto suffix=std::to_string(i)+"_"+std::to_string(rng()%1000);
        out.emplace_back(p+"_"+suffix,types[rng()%types.size()],scopes[rng()%scopes.size()],static_cast<int>(1+rng()%5000));
    }
    std::shuffle(out.begin(),out.end(),rng);
    return out;
}
static Summary summarize(std::vector<double> x) {
    if (x.empty()) return {};
    auto medianOf = [](std::vector<double> values) {
        std::sort(values.begin(), values.end());
        const size_t n=values.size();
        return n%2 ? values[n/2] : (values[n/2-1]+values[n/2])/2.0;
    };
    const double median = medianOf(x);
    if (x.size() < 2) return {median, 0.0};
    // Percentile bootstrap CI for the median (not a t interval for the mean).
    // Fixed seed makes the summary reproducible across reruns of the same sample.
    std::mt19937 rng(0xB00757A9u + static_cast<uint32_t>(x.size()));
    std::uniform_int_distribution<size_t> pick(0, x.size()-1);
    std::vector<double> boot; boot.reserve(4000);
    std::vector<double> sample(x.size());
    for (size_t b=0; b<4000; ++b) {
        for (double& v : sample) v=x[pick(rng)];
        boot.push_back(medianOf(sample));
    }
    std::sort(boot.begin(), boot.end());
    const double low=boot[static_cast<size_t>(0.025*(boot.size()-1))];
    const double high=boot[static_cast<size_t>(0.975*(boot.size()-1))];
    // CSV stores a symmetric half-width for convenient error-bar rendering;
    // it conservatively covers the wider side of the percentile interval.
    return {median, std::max(median-low, high-median)};
}

static double percentile(std::vector<size_t> x, double p) {
    if(x.empty()) return 0;
    std::sort(x.begin(),x.end());
    size_t idx=static_cast<size_t>(std::ceil(p*x.size()));
    if(idx==0) idx=1;
    return static_cast<double>(x[std::min(idx-1,x.size()-1)]);
}
static Result benchmark(HashType ht, CollisionMethod cm, double load, int repetitions, uint32_t seedBase) {
    const size_t capacity=8192; // power of two supports full triangular probing
    const size_t n=static_cast<size_t>(capacity*load);
    std::vector<double> ins, succ, unsucc;
    std::vector<size_t> missProbes;
    size_t lastCollisions=0,lastTotalProbes=0,lastMaxDistance=0,lastRehashCount=0,lastPeakBytes=0,inserted=0,failed=0,failedSearches=0;
    double lastRehashMs=0;
    double actualLoad=0,avgProbes=0;
    for(int rep=0;rep<repetitions;++rep){
        auto symbols=makeWorkload(n,seedBase+static_cast<uint32_t>(rep));
        SymbolTable table(capacity,ht,cm);
        auto a=Clock::now();
        for(const auto& s:symbols) if(!table.insert(s)) ++failed;
        auto b=Clock::now();
        Symbol result;
        auto c=Clock::now();
        for(const auto& s:symbols) if(!table.search(s.name,result)) ++failedSearches;
        auto d=Clock::now();
        std::vector<std::string> missing; missing.reserve(n);
        for(size_t i=0;i<n;++i) missing.push_back("absent_identifier_"+std::to_string(seedBase)+"_"+std::to_string(rep)+"_"+std::to_string(i));
        std::mt19937 rng(seedBase+rep+100000); std::shuffle(missing.begin(),missing.end(),rng);
        auto e=Clock::now();
        for(const auto& key:missing) { Symbol ignored; table.search(key,ignored); missProbes.push_back(table.getLastSearchProbeCount()); }
        auto f=Clock::now();
        ins.push_back(std::chrono::duration<double,std::milli>(b-a).count());
        succ.push_back(std::chrono::duration<double,std::milli>(d-c).count());
        unsucc.push_back(std::chrono::duration<double,std::milli>(f-e).count());
        lastCollisions=table.getCollisionCount(); lastTotalProbes=table.getTotalProbes();
        lastMaxDistance=table.getMaxProbeDistance(); lastRehashCount=table.getRehashCount(); lastPeakBytes=table.getPeakTableArrayBytes(); lastRehashMs=table.getRehashDurationMs(); inserted=table.getElementCount();
        actualLoad=table.getLoadFactor(); avgProbes=table.getAverageProbes();
    }
    const Summary si=summarize(ins), ss=summarize(succ), su=summarize(unsucc);
    Result r; r.hash=hashTypeToString(ht); r.method=collisionMethodToString(cm); r.targetLoad=load; r.actualLoad=actualLoad;
    r.insertionMedian=si.median; r.insertionCI=si.ci95; r.successfulMedian=ss.median; r.successfulCI=ss.ci95;
    r.unsuccessfulMedian=su.median; r.unsuccessfulCI=su.ci95;
    r.unsuccessfulMeanProbes=missProbes.empty()?0:std::accumulate(missProbes.begin(),missProbes.end(),0.0)/missProbes.size();
    r.unsuccessfulP50=percentile(missProbes,.50); r.unsuccessfulP95=percentile(missProbes,.95);
    r.unsuccessfulMax=missProbes.empty()?0:*std::max_element(missProbes.begin(),missProbes.end());
    r.collisions=lastCollisions; r.totalProbes=lastTotalProbes; r.averageProbes=avgProbes; r.maxProbeDistance=lastMaxDistance;
    r.repetitions=repetitions; r.inserted=inserted; r.failedInserts=failed; r.failedSearches=failedSearches; r.rehashCount=lastRehashCount; r.tableArrayBytes=capacity*sizeof(TableEntry); r.peakTableArrayBytes=lastPeakBytes; r.bytesPerSlot=sizeof(TableEntry); r.rehashDurationMs=lastRehashMs; return r;
}

static void writeResults(const fs::path& path,const std::vector<Result>& rs) {
    std::ofstream f(path);
    f<<"HashFunction,CollisionMethod,LoadFactor,ActualLoadFactor,InsertionTimeMedianMs,InsertionTimeCI95Ms,SuccessfulSearchMedianMs,SuccessfulSearchCI95Ms,UnsuccessfulSearchMedianMs,UnsuccessfulSearchCI95Ms,InsertionTime,SuccessfulSearchTime,UnsuccessfulSearchTime,UnsuccessfulMeanProbes,UnsuccessfulP50Probes,UnsuccessfulP95Probes,UnsuccessfulMaxProbes,Collisions,TotalProbes,AverageProbes,MaxProbeDistance,Repetitions,Inserted,FailedInserts,FailedSearches,RehashCount,TableArrayBytes,PeakTableArrayBytes,BytesPerSlot,RehashDurationMs\n";
    f<<std::setprecision(10);
    for(const auto&r:rs) f<<r.hash<<','<<r.method<<','<<r.targetLoad<<','<<r.actualLoad<<','<<r.insertionMedian<<','<<r.insertionCI<<','<<r.successfulMedian<<','<<r.successfulCI<<','<<r.unsuccessfulMedian<<','<<r.unsuccessfulCI<<','<<r.insertionMedian<<','<<r.successfulMedian<<','<<r.unsuccessfulMedian<<','<<r.unsuccessfulMeanProbes<<','<<r.unsuccessfulP50<<','<<r.unsuccessfulP95<<','<<r.unsuccessfulMax<<','<<r.collisions<<','<<r.totalProbes<<','<<r.averageProbes<<','<<r.maxProbeDistance<<','<<r.repetitions<<','<<r.inserted<<','<<r.failedInserts<<','<<r.failedSearches<<','<<r.rehashCount<<','<<r.tableArrayBytes<<','<<r.peakTableArrayBytes<<','<<r.bytesPerSlot<<','<<r.rehashDurationMs<<'\n';
}

static void writeStdBaseline(const fs::path& path,int repetitions) {
    std::ofstream f(path);
    f << "Baseline,LoadFactor,Repetitions,InsertionMedianMs,InsertionCI95Ms,SuccessfulSearchMedianMs,SuccessfulSearchCI95Ms,UnsuccessfulSearchMedianMs,UnsuccessfulSearchCI95Ms,AverageBucketLoad,FinalSize\n";
    const size_t capacity=8192;
    for(double load:{.25,.40,.50,.60,.70,.80}) {
        const size_t n=static_cast<size_t>(capacity*load);
        std::vector<double> ins,succ,unsucc; double bucketLoad=0; size_t finalSize=0;
        for(int rep=0;rep<repetitions;++rep) {
            auto symbols=makeWorkload(n,99173u+static_cast<uint32_t>(rep));
            std::unordered_map<std::string,Symbol> table; table.max_load_factor(1.0f); table.reserve(n);
            std::vector<std::string> missing; missing.reserve(n);
            for(size_t i=0;i<n;++i) missing.push_back("absent_identifier_"+std::to_string(rep)+"_"+std::to_string(i));
            auto a=Clock::now(); for(const auto& sym:symbols) table.emplace(sym.name,sym); auto b=Clock::now();
            Symbol found; auto c=Clock::now(); for(const auto& sym:symbols){auto it=table.find(sym.name);if(it!=table.end())found=it->second;} auto d=Clock::now();
            auto e=Clock::now(); for(const auto& key:missing) (void)table.find(key); auto z=Clock::now();
            ins.push_back(std::chrono::duration<double,std::milli>(b-a).count());
            succ.push_back(std::chrono::duration<double,std::milli>(d-c).count());
            unsucc.push_back(std::chrono::duration<double,std::milli>(z-e).count());
            bucketLoad=table.load_factor(); finalSize=table.size();
        }
        const auto a=summarize(ins),b=summarize(succ),c=summarize(unsucc);
        f<<"std::unordered_map,"<<load<<','<<repetitions<<','<<a.median<<','<<a.ci95<<','<<b.median<<','<<b.ci95<<','<<c.median<<','<<c.ci95<<','<<bucketLoad<<','<<finalSize<<'\n';
    }
}

static void writeModernBaseline(const fs::path& path, int repetitions) {
    std::ofstream f(path);
    f << "Baseline,LoadFactor,Repetitions,InsertionMedianMs,InsertionCI95Ms,SuccessfulSearchMedianMs,SuccessfulSearchCI95Ms,UnsuccessfulSearchMedianMs,UnsuccessfulSearchCI95Ms,FinalSize\n";
#if HAS_BOOST_FLAT_MAP
    const size_t capacity=8192;
    for(double load:{.25,.40,.50,.60,.70,.80}) {
        const size_t n=static_cast<size_t>(capacity*load);
        std::vector<double> ins,succ,unsucc; size_t finalSize=0;
        for(int rep=0;rep<repetitions;++rep) {
            auto symbols=makeWorkload(n,99173u+static_cast<uint32_t>(rep));
            boost::unordered_flat_map<std::string,Symbol> table;
            table.reserve(n);
            std::vector<std::string> missing; missing.reserve(n);
            for(size_t i=0;i<n;++i) missing.push_back("absent_identifier_"+std::to_string(rep)+"_"+std::to_string(i));
            auto a=Clock::now(); for(const auto& sym:symbols) table.emplace(sym.name,sym); auto b=Clock::now();
            auto c=Clock::now(); for(const auto& sym:symbols) (void)table.find(sym.name); auto d=Clock::now();
            auto e=Clock::now(); for(const auto& key:missing) (void)table.find(key); auto z=Clock::now();
            ins.push_back(std::chrono::duration<double,std::milli>(b-a).count());
            succ.push_back(std::chrono::duration<double,std::milli>(d-c).count());
            unsucc.push_back(std::chrono::duration<double,std::milli>(z-e).count());
            finalSize=table.size();
        }
        const auto a=summarize(ins),b=summarize(succ),c=summarize(unsucc);
        f<<"Boost.UnorderedFlatMap,"<<load<<','<<repetitions<<','<<a.median<<','<<a.ci95<<','<<b.median<<','<<b.ci95<<','<<c.median<<','<<c.ci95<<','<<finalSize<<'\n';
    }
#else
    f << "Boost.UnorderedFlatMap,UNAVAILABLE,0,0,0,0,0,0,0,0\n";
#endif
}

static void writeHashQuality(const fs::path& path) {
    const size_t n=50000,buckets=8192;
    std::vector<std::string> keys; keys.reserve(n);
    for(size_t i=0;i<n;++i) keys.push_back("identifier_"+std::to_string(i));
    std::ofstream f(path);
    f<<"HashFunction,Keys,Buckets,OccupiedBuckets,CollisionPairs,MaxBucketOccupancy,MeanBucketOccupancy,ChiSquareStatistic,DegreesOfFreedom,ChiSquarePValueApprox\n";
    for(HashType ht:{HashType::MURMURHASH3,HashType::FNV1A_AVALANCHE,HashType::XXHASH}){
        std::vector<size_t> counts(buckets,0);
        for(const auto& k:keys) ++counts[getHash(k,ht)%buckets];
        size_t occupied=0,maxBucket=0;
        long double pairs=0,chiSquare=0;
        const long double expected=static_cast<long double>(n)/buckets;
        for(size_t c:counts){
            if(c)++occupied;
            maxBucket=std::max(maxBucket,c);
            pairs+=static_cast<long double>(c)*(c-1)/2;
            const long double delta=static_cast<long double>(c)-expected;
            chiSquare+=delta*delta/expected;
        }
        f<<hashTypeToString(ht)<<','<<n<<','<<buckets<<','<<occupied<<','
         <<static_cast<uint64_t>(pairs)<<','<<maxBucket<<','<<static_cast<double>(n)/buckets<<','
         <<static_cast<double>(chiSquare)<<','<<(buckets-1)<<','<<std::clamp(std::erfc((std::pow(static_cast<double>(chiSquare)/(buckets-1),1.0/3.0)-(1.0-2.0/(9.0*(buckets-1))))/std::sqrt(2.0/(9.0*(buckets-1)))/std::sqrt(2.0)),0.0,1.0)<<'\n';
    }
}

static void writeHashTiming(const fs::path& path, int repetitions) {
    std::vector<std::string> keys;
    for (size_t i=0;i<50000;++i) keys.push_back("identifier_"+std::to_string(i)+"_compiler_scope");
    std::ofstream f(path);
    f << "HashFunction,Repetitions,KeysPerRun,MedianMs,CI95Ms,MedianNanosecondsPerHash\n";
    for (HashType ht : {HashType::MURMURHASH3, HashType::FNV1A_AVALANCHE, HashType::XXHASH}) {
        std::vector<double> times;
        static volatile uint64_t observableHashSink = 0;
        uint64_t sink=0;
        for (int rep=0; rep<repetitions; ++rep) {
            sink=0;
            auto start=Clock::now();
            for (const auto& key:keys) sink ^= getHash(key,ht);
            auto stop=Clock::now();
            observableHashSink = sink; // one observable write per repetition, outside the timed loop
            times.push_back(std::chrono::duration<double,std::milli>(stop-start).count());
        }
        const auto summary=summarize(times);
        f<<hashTypeToString(ht)<<','<<repetitions<<','<<keys.size()<<','<<summary.median<<','<<summary.ci95<<','
         <<(summary.median*1e6/keys.size())<<'\n';
        (void)sink;
    }
}

static void writeAblation(const fs::path& path, int repetitions) {
    struct Variant { std::string name; CollisionMethod method; SymbolTableOptions options; };
    SymbolTableOptions full;
    SymbolTableOptions noKeyword=full; noKeyword.rejectKeywords=false;
    SymbolTableOptions noPrecheck=full; noPrecheck.precheckDuplicates=false; noPrecheck.unsafeAssumeUniqueKeysForBenchmark=true;
    SymbolTableOptions noSwitch=full; noSwitch.adaptiveHybridSwitch=false;
    std::vector<Variant> variants={
        {"Hybrid full",CollisionMethod::HYBRID,full},
        {"Hybrid without keyword filter",CollisionMethod::HYBRID,noKeyword},
        {"Hybrid without duplicate checks (unique keys)",CollisionMethod::HYBRID,noPrecheck},
        {"Hybrid without adaptive switch",CollisionMethod::HYBRID,noSwitch},
        {"Robin Hood double hashing from first insertion",CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING,full},
        {"Double hashing baseline (no adaptive phase)",CollisionMethod::DOUBLE_HASHING,full}
    };
    struct Row {
        std::string variant; double load, insertMedian, insertCI, keywordMedian, keywordCI;
        size_t rejectedKeywords, acceptedKeywords, failedSearches; int reps;
    };
    const size_t capacity=4096, n=1800;
    const std::vector<std::string> keywords={"int","return","if","while","class","struct","for","const","void","switch",
        "float","double","static","delete","new","true","false","break","continue","char"};
    std::vector<std::vector<double>> identifierTimes(variants.size()), keywordTimes(variants.size());
    std::vector<size_t> rejected(variants.size(),0), accepted(variants.size(),0), failedSearches(variants.size(),0);
    std::mt19937 orderRng(770031u);
    for(int rep=0;rep<repetitions;++rep) {
        std::vector<size_t> order(variants.size());
        std::iota(order.begin(),order.end(),0);
        std::shuffle(order.begin(),order.end(),orderRng);
        auto symbols=makeWorkload(n,80001u+static_cast<uint32_t>(rep));
        std::mt19937 rng(9000u+static_cast<uint32_t>(rep));
        std::shuffle(symbols.begin(),symbols.end(),rng);
        for(size_t vi:order) {
            const auto& variant=variants[vi];
            SymbolTable table(capacity,HashType::MURMURHASH3,variant.method,variant.options);
            auto start=Clock::now();
            for(const auto& sym:symbols) (void)table.insert(sym);
            auto stop=Clock::now();
            identifierTimes[vi].push_back(std::chrono::duration<double,std::milli>(stop-start).count());
            Symbol found;
            for(const auto& sym:symbols) if(!table.search(sym.name,found)) ++failedSearches[vi];

            // Isolate keyword handling in a fresh table so accepted keyword
            // entries cannot contaminate the identifier-only timing/load factor.
            SymbolTable keywordTable(capacity,HashType::MURMURHASH3,variant.method,variant.options);
            auto ka=Clock::now();
            for(size_t k=0;k<keywords.size();++k) {
                const bool inserted=keywordTable.insert(Symbol(keywords[k],"keyword","lexer_trace",static_cast<int>(k+1)));
                if(inserted) ++accepted[vi];
                else if(variant.options.rejectKeywords) ++rejected[vi];
            }
            auto kb=Clock::now();
            keywordTimes[vi].push_back(std::chrono::duration<double,std::milli>(kb-ka).count());
        }
    }
    std::ofstream f(path);
    f<<"Variant,TargetLoadFactor,Repetitions,IdentifierOnlyMedianMs,IdentifierOnlyCI95Ms,KeywordAttemptsMedianMs,KeywordAttemptsCI95Ms,RejectedKeywordAttempts,AcceptedKeywordAttempts,FailedLookups\n";
    for(size_t vi=0;vi<variants.size();++vi) {
        const auto insertSummary=summarize(identifierTimes[vi]), keywordSummary=summarize(keywordTimes[vi]);
        f<<variants[vi].name<<','<<static_cast<double>(n)/capacity<<','<<repetitions<<','
         <<insertSummary.median<<','<<insertSummary.ci95<<','<<keywordSummary.median<<','<<keywordSummary.ci95<<','
         <<rejected[vi]<<','<<accepted[vi]<<','<<failedSearches[vi]<<'\n';
    }
}

static void writeSourceTraceBenchmark(const fs::path& tracePath, const fs::path& outPath, int repetitions) {
    struct Token { std::string file, name; int line=0, depth=0, keyword=0; };
    std::ifstream in(tracePath);
    std::vector<Token> tokens;
    std::string line;
    if (!in) return;
    std::getline(in,line); // header
    while(std::getline(in,line)) {
        std::stringstream ss(line); std::string field; Token t;
        if(!std::getline(ss,t.file,',')) continue;
        if(!std::getline(ss,field,',')) continue; try { t.line=std::stoi(field); } catch(...) { continue; }
        if(!std::getline(ss,t.name,',')) continue;
        if(!std::getline(ss,field,',')) continue; try { t.depth=std::stoi(field); } catch(...) { t.depth=0; }
        if(!std::getline(ss,field,',')) continue; try { t.keyword=std::stoi(field); } catch(...) { t.keyword=0; }
        tokens.push_back(t);
    }
    std::ofstream f(outPath);
    f<<"CollisionMethod,Repetitions,TokenOccurrences,KeywordTokens,UniqueScopeIdentifiers,MedianTraceMs,CI95Ms,FailedOperations,ScopeTransitions,DynamicRehashes\n";
    if(tokens.empty()) return;
    std::vector<CollisionMethod> methods={CollisionMethod::LINEAR,CollisionMethod::QUADRATIC,
        CollisionMethod::DOUBLE_HASHING,CollisionMethod::ROBIN_HOOD,
        CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING,CollisionMethod::HYBRID};
    for(auto method:methods) {
        std::vector<double> times; size_t failed=0, keywordCount=0, uniqueCount=0, transitions=0, rehashes=0;
        for(int rep=0;rep<repetitions;++rep) {
            SymbolTableOptions opts; opts.autoGrow=true; opts.maxLoadFactor=0.70;
            ScopedSymbolEnvironment env(16,HashType::MURMURHASH3,method,opts);
            std::vector<std::unordered_set<std::string>> seen(1);
            std::string activeFile; int currentDepth=0; bool fileScopeActive=false;
            auto start=Clock::now();
            for(const auto& token:tokens) {
                if(token.file!=activeFile) {
                    while(currentDepth>0){ env.exitScope(); --currentDepth; ++transitions; }
                    if(fileScopeActive){ env.exitScope(); ++transitions; }
                    activeFile=token.file; seen.clear(); seen.emplace_back();
                    env.enterScope("translation_unit_"+activeFile); fileScopeActive=true; ++transitions;
                }
                const int wanted=std::min(token.depth,64);
                while(currentDepth<wanted){ ++currentDepth; env.enterScope(activeFile+"_depth_"+std::to_string(currentDepth)); seen.emplace_back(); ++transitions; }
                while(currentDepth>wanted && currentDepth>0){ env.exitScope(); --currentDepth; if(seen.size()>1)seen.pop_back(); ++transitions; }
                if(token.keyword){ ++keywordCount; continue; } // lexical layer consumes keywords
                if(seen.back().insert(token.name).second) {
                    if(!env.insert(Symbol(token.name,"identifier",activeFile,token.line))) ++failed;
                    else ++uniqueCount;
                } else {
                    Symbol found; if(!env.search(token.name,found)) ++failed;
                }
            }
            auto stop=Clock::now();
            times.push_back(std::chrono::duration<double,std::milli>(stop-start).count());
            rehashes += env.getTotalRehashCount();
        }
        auto summary=summarize(times);
        f<<collisionMethodToString(method)<<','<<repetitions<<','<<tokens.size()<<','<<(keywordCount/repetitions)<<','<<(uniqueCount/repetitions)<<','
         <<summary.median<<','<<summary.ci95<<','<<failed<<','<<(transitions/repetitions)<<','<<(rehashes/repetitions)<<'\n';
    }
}

static void writeExampleTable(const fs::path& path) {
    SymbolTable table(32,HashType::MURMURHASH3,CollisionMethod::ROBIN_HOOD);
    for(const Symbol&s:std::vector<Symbol>{{"count","int","global",1},{"total","float","global",2},{"average","float","function_main",3},{"result","int","block_1",4},{"studentCount","int","block_1",5},{"salary","float","global",6},{"age","int","global",7},{"grade","char","function_main",8}}) table.insert(s);
    std::ofstream f(path); f<<"Index,Identifier,Type,Scope,Line,Hash,ProbeDistance\n";
    const auto& entries=table.getTable(); for(size_t i=0;i<entries.size();++i){if(entries[i].occupied)f<<i<<','<<entries[i].symbol.name<<','<<entries[i].symbol.type<<','<<entries[i].symbol.scope<<','<<entries[i].symbol.line<<','<<entries[i].hashValue<<','<<entries[i].probeDistance<<'\n';else f<<i<<",,,,,,\n";}
}
int main(int argc,char**argv) {
    int repetitions=10; if(argc>1) repetitions=std::max(1,std::min(30,std::stoi(argv[1])));
    fs::path root=fs::current_path(); if(root.filename()=="backend") root=root.parent_path();
    const fs::path data=root/"data"; fs::create_directories(data);
    const std::vector<HashType> hashes={HashType::MURMURHASH3,HashType::FNV1A_AVALANCHE,HashType::XXHASH};
    const std::vector<CollisionMethod> methods={CollisionMethod::LINEAR,CollisionMethod::QUADRATIC,CollisionMethod::DOUBLE_HASHING,CollisionMethod::ROBIN_HOOD,CollisionMethod::ROBIN_HOOD_DOUBLE_HASHING,CollisionMethod::HYBRID};
    const std::vector<double> loads={.25,.40,.50,.60,.70,.80};
    struct Config { HashType hash; CollisionMethod method; double load; };
    std::vector<Config> configs;
    for(auto h:hashes) for(auto m:methods) for(double load:loads) configs.push_back({h,m,load});
    std::mt19937 orderRng(20261028u);
    std::shuffle(configs.begin(),configs.end(),orderRng);
    std::vector<Result> results; const size_t total=configs.size(); size_t current=0;
    std::cout<<"Compiler Symbol Table Benchmark | repetitions="<<repetitions<<"\\n";
    for(const auto& cfg:configs){
        std::cout<<"["<<++current<<'/'<<total<<"] "<<hashTypeToString(cfg.hash)<<" + "<<collisionMethodToString(cfg.method)<<" at "<<cfg.load*100<<"%\\n";
        results.push_back(benchmark(cfg.hash,cfg.method,cfg.load,repetitions,20261028u+static_cast<uint32_t>(current*31)));
    }
    writeResults(data/"results.csv",results);
    writeStdBaseline(data/"baseline_results.csv",repetitions);
    writeModernBaseline(data/"modern_baseline_results.csv",repetitions);
    writeHashQuality(data/"hash_quality.csv");
    writeHashTiming(data/"hash_timing.csv",repetitions);
    writeAblation(data/"ablation_results.csv",repetitions);
    writeSourceTraceBenchmark(data/"source_identifier_trace.csv",data/"source_trace_benchmark.csv",repetitions);
    writeExampleTable(data/"table.csv");
    std::cout<<"Wrote benchmark, ablation, hash timing, quality, and example-table CSV files.\n";
    return 0;
}
