
#ifndef CACHE_H
#define CACHE_H

#include <cstdint>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>

class Cache {
private:
    struct CacheLine {
        bool valid = false;
        std::uint64_t tag = 0;
        std::uint64_t lastUsed = 0;
    };

    int capacityBytes;
    int blockSizeBytes;
    int associativity;
    int numberOfSets;

    std::vector<std::vector<CacheLine>> sets;

    std::uint64_t clock = 0;
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;

public:
    // associativity = 1: direct mapped
    // associativity = number of lines: fully associative
    // otherwise: N-way set associative
    Cache(int capacityBytes, int blockSizeBytes, int associativity)
        : capacityBytes(capacityBytes),
          blockSizeBytes(blockSizeBytes),
          associativity(associativity),
          numberOfSets(0) {

        if (capacityBytes <= 0 || blockSizeBytes <= 0 ||
            associativity <= 0) {
            throw std::invalid_argument(
                "Cache capacity, block size and associativity must be positive."
            );
        }

        if (capacityBytes % blockSizeBytes != 0) {
            throw std::invalid_argument(
                "Cache capacity must be divisible by block size."
            );
        }

        const int numberOfLines = capacityBytes / blockSizeBytes;

        if (associativity > numberOfLines ||
            numberOfLines % associativity != 0) {
            throw std::invalid_argument(
                "Associativity must divide the number of cache lines."
            );
        }

        numberOfSets = numberOfLines / associativity;
        sets.resize(numberOfSets);

        for (auto& set : sets) {
            set.resize(associativity);
        }
    }

    // Access a memory address.
    // Returns true on hit and false on miss.
    bool access(int byteAddress) {
        if (byteAddress < 0) {
            throw std::invalid_argument("Memory address cannot be negative.");
        }

        ++clock;

        const std::uint64_t address =
            static_cast<std::uint64_t>(byteAddress);

        const std::uint64_t blockAddress =
            address / static_cast<std::uint64_t>(blockSizeBytes);

        const int setIndex = static_cast<int>(
            blockAddress % static_cast<std::uint64_t>(numberOfSets)
        );

        const std::uint64_t tag =
            blockAddress / static_cast<std::uint64_t>(numberOfSets);

        auto& set = sets[setIndex];

        // Look for a matching valid cache line.
        for (auto& line : set) {
            if (line.valid && line.tag == tag) {
                ++hits;
                line.lastUsed = clock;
                return true;
            }
        }

        // No matching line: cache miss.
        ++misses;

        // First preference: an invalid line.
        for (auto& line : set) {
            if (!line.valid) {
                line.valid = true;
                line.tag = tag;
                line.lastUsed = clock;
                return false;
            }
        }

        // Set is full: evict the least recently used line.
        auto lruLine = std::min_element(
            set.begin(),
            set.end(),
            [](const CacheLine& a, const CacheLine& b) {
                return a.lastUsed < b.lastUsed;
            }
        );

        lruLine->valid = true;
        lruLine->tag = tag;
        lruLine->lastUsed = clock;

        return false;
    }

    std::uint64_t getHits() const {
        return hits;
    }

    std::uint64_t getMisses() const {
        return misses;
    }

    std::uint64_t getAccesses() const {
        return hits + misses;
    }

    double getHitRate() const {
        const auto accesses = getAccesses();

        if (accesses == 0) {
            return 0.0;
        }

        return static_cast<double>(hits) / accesses;
    }

    double getMissRate() const {
        const auto accesses = getAccesses();

        if (accesses == 0) {
            return 0.0;
        }

        return static_cast<double>(misses) / accesses;
    }

    int getCapacityBytes() const {
        return capacityBytes;
    }

    int getBlockSizeBytes() const {
        return blockSizeBytes;
    }

    int getAssociativity() const {
        return associativity;
    }

    int getNumberOfSets() const {
        return numberOfSets;
    }

    void resetStatistics() {
        hits = 0;
        misses = 0;
    }

    // Clear cache contents and statistics.
    void clear() {
        for (auto& set : sets) {
            for (auto& line : set) {
                line.valid = false;
                line.tag = 0;
                line.lastUsed = 0;
            }
        }

        clock = 0;
        hits = 0;
        misses = 0;
    }

    void printStatistics() const {
        std::cout << "\n========== CACHE STATISTICS ==========\n";
        std::cout << "Capacity:       " << capacityBytes << " bytes\n";
        std::cout << "Block size:     " << blockSizeBytes << " bytes\n";
        std::cout << "Associativity:  " << associativity << "-way\n";
        std::cout << "Number of sets: " << numberOfSets << '\n';
        std::cout << "Total accesses: " << getAccesses() << '\n';
        std::cout << "Cache hits:     " << hits << '\n';
        std::cout << "Cache misses:   " << misses << '\n';

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Hit rate:       " << getHitRate() * 100.0 << "%\n";
        std::cout << "Miss rate:      " << getMissRate() * 100.0 << "%\n";
        std::cout << "======================================\n";
    }
};

#endif
