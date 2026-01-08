#include "StatsManager.h"
#include <iomanip>

StatsManager::StatsManager()
    : totalAllocations(0), successfulAllocations(0), failedAllocations(0),
      internalFragmentation(0.0), externalFragmentation(0.0), memoryUtilization(0.0),
      totalMemory(0), usedMemory(0), freeMemory(0),
      l1CacheHits(0), l1CacheMisses(0), l2CacheHits(0), l2CacheMisses(0),
      pageFaults(0), pageHits(0) {
}

void StatsManager::logMemoryAllocation(size_t size, bool success) {
    totalAllocations++;
    if (success) {
        successfulAllocations++;
    } else {
        failedAllocations++;
    }
}

void StatsManager::logCacheAccess(size_t level, bool hit) {
    if (level == 1) {
        if (hit) {
            l1CacheHits++;
        } else {
            l1CacheMisses++;
        }
    } else if (level == 2) {
        if (hit) {
            l2CacheHits++;
        } else {
            l2CacheMisses++;
        }
    }
}

void StatsManager::logPageFault() {
    pageFaults++;
}

void StatsManager::logPageHit() {
    pageHits++;
}

void StatsManager::setCacheStats(size_t l1Hits, size_t l1Misses, size_t l2Hits, size_t l2Misses) {
    l1CacheHits = l1Hits;
    l1CacheMisses = l1Misses;
    l2CacheHits = l2Hits;
    l2CacheMisses = l2Misses;
}

void StatsManager::setFragmentationMetrics(double internal, double external, double utilization) {
    internalFragmentation = internal;
    externalFragmentation = external;
    memoryUtilization = utilization;
}

void StatsManager::setMemoryStats(size_t total, size_t used, size_t free) {
    totalMemory = total;
    usedMemory = used;
    freeMemory = free;
}

void StatsManager::printStats() const {
    std::cout << "\n=== Simulation Statistics ===\n";
    
    std::cout << "\nMemory Allocation:\n";
    std::cout << "  Total Allocations: " << totalAllocations << "\n";
    std::cout << "  Successful: " << successfulAllocations << "\n";
    std::cout << "  Failed: " << failedAllocations << "\n";
    if (totalAllocations > 0) {
        double successRate = (static_cast<double>(successfulAllocations) / totalAllocations) * 100.0;
        std::cout << "  Success Rate: " << std::fixed << std::setprecision(2) << successRate << "%\n";
    }
    
    std::cout << "\nMemory Usage:\n";
    std::cout << "  Total Memory: " << totalMemory << " bytes\n";
    std::cout << "  Used Memory: " << usedMemory << " bytes\n";
    std::cout << "  Free Memory: " << freeMemory << " bytes\n";
    std::cout << "  Memory Utilization: " << std::fixed << std::setprecision(2) 
              << memoryUtilization << "%\n";
    
    std::cout << "\nFragmentation:\n";
    std::cout << "  Internal Fragmentation: " << std::fixed << std::setprecision(2) 
              << internalFragmentation << "%\n";
    std::cout << "  External Fragmentation: " << std::fixed << std::setprecision(2) 
              << externalFragmentation << "%\n";
    
    std::cout << "\nCache Statistics (L1):\n";
    size_t l1Total = l1CacheHits + l1CacheMisses;
    std::cout << "  Hits: " << l1CacheHits << "\n";
    std::cout << "  Misses: " << l1CacheMisses << "\n";
    if (l1Total > 0) {
        double l1HitRatio = (static_cast<double>(l1CacheHits) / l1Total) * 100.0;
        std::cout << "  Hit Ratio: " << std::fixed << std::setprecision(2) << l1HitRatio << "%\n";
    }
    
    std::cout << "\nCache Statistics (L2):\n";
    size_t l2Total = l2CacheHits + l2CacheMisses;
    std::cout << "  Hits: " << l2CacheHits << "\n";
    std::cout << "  Misses: " << l2CacheMisses << "\n";
    if (l2Total > 0) {
        double l2HitRatio = (static_cast<double>(l2CacheHits) / l2Total) * 100.0;
        std::cout << "  Hit Ratio: " << std::fixed << std::setprecision(2) << l2HitRatio << "%\n";
    }
    
    std::cout << "\nVirtual Memory:\n";
    size_t vmTotal = pageFaults + pageHits;
    std::cout << "  Page Faults: " << pageFaults << "\n";
    std::cout << "  Page Hits: " << pageHits << "\n";
    if (vmTotal > 0) {
        double faultRate = (static_cast<double>(pageFaults) / vmTotal) * 100.0;
        std::cout << "  Page Fault Rate: " << std::fixed << std::setprecision(2) << faultRate << "%\n";
    }
    
    std::cout << "============================\n\n";
}
