#ifndef STATS_MANAGER_H
#define STATS_MANAGER_H

#include <cstddef>
#include <iostream>

class StatsManager {
public:
    StatsManager();

    void logMemoryAllocation(size_t size, bool success);
    void logCacheAccess(size_t level, bool hit);
    void setCacheStats(size_t l1Hits, size_t l1Misses, size_t l2Hits, size_t l2Misses); // New sync method
    void logPageFault();
    void logPageHit();
    
    void setFragmentationMetrics(double internal, double external, double utilization);
    void setMemoryStats(size_t total, size_t used, size_t free);

    void printStats() const;

private:
    // Memory allocation statistics
    size_t totalAllocations;
    size_t successfulAllocations;
    size_t failedAllocations;
    
    // Fragmentation metrics
    double internalFragmentation;
    double externalFragmentation;
    double memoryUtilization;
    size_t totalMemory;
    size_t usedMemory;
    size_t freeMemory;
    
    // Cache statistics
    size_t l1CacheHits;
    size_t l1CacheMisses;
    size_t l2CacheHits;
    size_t l2CacheMisses;
    
    // Virtual memory statistics
    size_t pageFaults;
    size_t pageHits;
};

#endif // STATS_MANAGER_H
