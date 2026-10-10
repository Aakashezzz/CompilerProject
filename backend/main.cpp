#include "include/hash_functions.h"
#include "include/symbol_table.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <iomanip>

using namespace std;

vector<Symbol> generateSymbols(size_t count) {
    vector<Symbol> symbols;
    symbols.reserve(count);

    for (size_t i = 0; i < count; i++) {
        string name = "variable_" + to_string(i);

        string type;

        if (i % 3 == 0)
            type = "int";
        else if (i % 3 == 1)
            type = "float";
        else
            type = "char";

        string scope = (i % 2 == 0) ? "global" : "local";

        symbols.emplace_back(
            name,
            type,
            scope,
            static_cast<int>(i + 1)
        );
    }

    return symbols;
}

struct BenchmarkResult {
    string hashFunction;
    string collisionMethod;
    double loadFactor;
    double insertionTime;
    double successfulSearchTime;
    double unsuccessfulSearchTime;
    size_t collisions;
    size_t totalProbes;
    double averageProbes;
    size_t maxProbeDistance;
};

BenchmarkResult runBenchmark(
    HashType hashType,
    CollisionMethod collisionMethod,
    double loadFactor
) {
    const size_t tableSize = 100003;
    size_t numberOfElements =
        static_cast<size_t>(tableSize * loadFactor);

    vector<Symbol> symbols =
        generateSymbols(numberOfElements);

    SymbolTable table(
        tableSize,
        hashType,
        collisionMethod
    );

    auto startInsert = chrono::high_resolution_clock::now();

    for (const auto& symbol : symbols) {
        table.insert(symbol);
    }

    auto endInsert = chrono::high_resolution_clock::now();

    double insertionTime =
        chrono::duration<double, milli>(
            endInsert - startInsert
        ).count();

    Symbol result;

    auto startSearch = chrono::high_resolution_clock::now();

    for (const auto& symbol : symbols) {
        table.search(symbol.name, result);
    }

    auto endSearch = chrono::high_resolution_clock::now();

    double successfulSearchTime =
        chrono::duration<double, milli>(
            endSearch - startSearch
        ).count();

    vector<string> missingKeys;

    for (size_t i = 0; i < numberOfElements; i++) {
        missingKeys.push_back(
            "missing_variable_" + to_string(i)
        );
    }

    auto startUnsuccessful =
        chrono::high_resolution_clock::now();

    for (const auto& key : missingKeys) {
        table.search(key, result);
    }

    auto endUnsuccessful =
        chrono::high_resolution_clock::now();

    double unsuccessfulSearchTime =
        chrono::duration<double, milli>(
            endUnsuccessful - startUnsuccessful
        ).count();

    BenchmarkResult resultData;

    resultData.hashFunction = table.getHashName();
    resultData.collisionMethod =
        table.getCollisionMethodName();

    resultData.loadFactor = loadFactor;
    resultData.insertionTime = insertionTime;
    resultData.successfulSearchTime =
        successfulSearchTime;
    resultData.unsuccessfulSearchTime =
        unsuccessfulSearchTime;

    resultData.collisions =
        table.getCollisionCount();

    resultData.totalProbes =
        table.getTotalProbes();

    resultData.averageProbes =
        table.getAverageProbes();

    resultData.maxProbeDistance =
        table.getMaxProbeDistance();

    return resultData;
}

void writeResults(
    const vector<BenchmarkResult>& results
) {
    ofstream file("../data/results.csv");

    file << "HashFunction,CollisionMethod,LoadFactor,"
         << "InsertionTime,SuccessfulSearchTime,"
         << "UnsuccessfulSearchTime,Collisions,"
         << "TotalProbes,AverageProbes,MaxProbeDistance\n";

    for (const auto& result : results) {
        file << result.hashFunction << ","
             << result.collisionMethod << ","
             << result.loadFactor << ","
             << result.insertionTime << ","
             << result.successfulSearchTime << ","
             << result.unsuccessfulSearchTime << ","
             << result.collisions << ","
             << result.totalProbes << ","
             << result.averageProbes << ","
             << result.maxProbeDistance
             << "\n";
    }

    file.close();
}

void writeExampleTable() {
    SymbolTable table(
        20,
        HashType::MURMURHASH3,
        CollisionMethod::ROBIN_HOOD
    );

    vector<Symbol> symbols = {
        {"count", "int", "global", 1},
        {"total", "float", "global", 2},
        {"average", "float", "local", 3},
        {"result", "int", "local", 4},
        {"studentCount", "int", "local", 5},
        {"salary", "float", "global", 6},
        {"age", "int", "global", 7},
        {"grade", "char", "local", 8}
    };

    for (const auto& symbol : symbols) {
        table.insert(symbol);
    }

    ofstream file("../data/table.csv");

    file << "Index,Identifier,Type,Scope,Line,Hash,ProbeDistance\n";

    const auto& entries = table.getTable();

    for (size_t i = 0; i < entries.size(); i++) {
        if (entries[i].occupied) {
            file << i << ","
                 << entries[i].symbol.name << ","
                 << entries[i].symbol.type << ","
                 << entries[i].symbol.scope << ","
                 << entries[i].symbol.line << ","
                 << entries[i].hashValue << ","
                 << entries[i].probeDistance
                 << "\n";
        }
        else {
            file << i
                 << ",,,,,,\n";
        }
    }

    file.close();
}

int main() {

    cout << "Compiler Symbol Table Performance Evaluation\n";
    cout << "=============================================\n\n";

    vector<HashType> hashFunctions = {
        HashType::MURMURHASH3,
        HashType::FXHASH,
        HashType::XXHASH
    };

        vector<CollisionMethod> collisionMethods = {
        CollisionMethod::LINEAR,
        CollisionMethod::QUADRATIC,
        CollisionMethod::DOUBLE_HASHING,
        CollisionMethod::ROBIN_HOOD,
        CollisionMethod::HYBRID
    };

    vector<double> loadFactors = {
        0.25,
        0.50,
        0.60,
        0.70,
        0.80,
        0.90
    };

    vector<BenchmarkResult> results;

    size_t totalTests =
        hashFunctions.size() *
        collisionMethods.size() *
        loadFactors.size();

    size_t currentTest = 0;

    for (HashType hashType : hashFunctions) {
        for (CollisionMethod collisionMethod : collisionMethods) {
            for (double loadFactor : loadFactors) {

                currentTest++;

                cout << "Running test "
                     << currentTest
                     << "/"
                     << totalTests
                     << " : "
                     << hashTypeToString(hashType)
                     << " + "
                     << collisionMethodToString(collisionMethod)
                     << " at "
                     << loadFactor * 100
                     << "%\n";

                results.push_back(
                    runBenchmark(
                        hashType,
                        collisionMethod,
                        loadFactor
                    )
                );
            }
        }
    }

    writeResults(results);
    writeExampleTable();

    cout << "\nBenchmark completed.\n";
    cout << "Results saved to data/results.csv\n";
    cout << "Example table saved to data/table.csv\n";

    return 0;
}