
#include <iostream>
#include <cmath>
#include "src/cpu/Cache.h"

int passed = 0;
int failed = 0;

void check(bool condition, const char* description) {
    if (condition) {
        ++passed;
        std::cout << "[PASS] " << description << '\n';
    } else {
        ++failed;
        std::cout << "[FAIL] " << description << '\n';
    }
}

int main() {
    std::cout << "Running cache tests...\n\n";

    // 1. One cache line, one 4-byte block.
    Cache directMapped(16, 4, 1);

    check(!directMapped.access(0), "First access is a miss");
    check(directMapped.access(0), "Repeated access is a hit");
    check(directMapped.access(1), "Address in the same block is a hit");
    check(directMapped.getHits() == 2, "Hit counter is correct");
    check(directMapped.getMisses() == 1, "Miss counter is correct");

    // 2. Addresses 0 and 16 map to the same set in this cache.
    check(!directMapped.access(16), "Conflicting address causes a miss");
    check(!directMapped.access(0), "Evicted address causes another miss");

    // 3. Two-way associative cache with two sets.
    Cache twoWay(16, 4, 2);

    check(!twoWay.access(0), "Two-way cache initial miss");
    check(!twoWay.access(8), "Second block in same set misses");
    check(twoWay.access(0), "Existing block hits in two-way cache");

    // 4. LRU replacement:
    // Capacity 8 bytes, block size 4, associativity 2.
    // Both blocks belong to the same set because there is one set.
    Cache fullyAssociative(8, 4, 2);

    check(!fullyAssociative.access(0), "LRU test: load block A");
    check(!fullyAssociative.access(4), "LRU test: load block B");
    check(fullyAssociative.access(0), "LRU test: A becomes most recently used");
    check(!fullyAssociative.access(8), "LRU test: C evicts least-recently-used B");
    check(fullyAssociative.access(0), "LRU test: A remains cached");
    check(!fullyAssociative.access(4), "LRU test: B was evicted");

    // 5. Statistics.
    Cache statsCache(16, 4, 1);
    statsCache.access(0);
    statsCache.access(0);
    statsCache.access(4);
    statsCache.access(4);

    check(statsCache.getAccesses() == 4, "Access count is correct");
    check(statsCache.getHits() == 2, "Statistics hit count is correct");
    check(statsCache.getMisses() == 2, "Statistics miss count is correct");
    check(std::abs(statsCache.getHitRate() - 0.5) < 1e-9,
          "Hit rate is 50 percent");
    check(std::abs(statsCache.getMissRate() - 0.5) < 1e-9,
          "Miss rate is 50 percent");

    // 6. Clear removes cache contents and statistics.
    statsCache.clear();
    check(statsCache.getAccesses() == 0, "Clear resets statistics");
    check(!statsCache.access(0), "Clear removes cached blocks");

    std::cout << "\nPassed: " << passed
              << " Failed: " << failed << '\n';

    return failed == 0 ? 0 : 1;
}
