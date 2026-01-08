#ifndef CACHE_SIMULATOR_H
#define CACHE_SIMULATOR_H

#include <cstddef>
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <string>

class CacheSimulator {
public:
    enum ReplacementPolicy {
        FIFO,
        LRU,
        LFU
    };

    CacheSimulator(size_t l1_size, size_t l1_block_size, size_t l1_associativity,
                   size_t l2_size, size_t l2_block_size, size_t l2_associativity,
                   ReplacementPolicy policy = FIFO);

    struct CacheAccessReport {
        bool l1Hit;
        bool l2Hit;
        bool l2Accessed;
        std::vector<std::string> events; // "Evicted L1 Tag X", "Filled L2", etc.
    };

    CacheAccessReport access(size_t physical_address);
    void setReplacementPolicy(ReplacementPolicy policy);
    void setReplacementPolicy(size_t level, ReplacementPolicy policy);
    
    // Statistics
    size_t getHits(size_t level) const;
    size_t getMisses(size_t level) const;
    double getHitRatio(size_t level) const;
    void printStatistics() const;

private:
    struct CacheBlock {
        bool valid;
        size_t tag;
        size_t load_time;      // For FIFO
        size_t last_access;     // For LRU
        size_t access_count;    // For LFU
    };

    struct CacheSet {
        std::vector<CacheBlock> blocks;
        size_t associativity;
        
        // FIFO: queue of block indices in order of insertion
        std::queue<size_t> fifoQueue;
        
        // LRU: list of block indices (most recently used first)
        std::list<size_t> lruList;
    };

    struct CacheLevel {
        size_t size;
        size_t block_size;
        size_t associativity;
        size_t num_sets;
        size_t set_index_bits;
        size_t block_offset_bits;
        size_t tag_bits;
        ReplacementPolicy policy;
        int levelNum; // 1 or 2
        
        std::vector<CacheSet> sets;
        
        size_t hits;
        size_t misses;
        size_t evictions;
        
        size_t global_time;  // For tracking access order
    };

    CacheLevel l1_cache;
    CacheLevel l2_cache;
    ReplacementPolicy defaultPolicy;

    void initCacheLevel(CacheLevel& level, int levelNum, size_t size, size_t block_size, 
                       size_t associativity, ReplacementPolicy policy);
    
    // update_stats: count hits/misses
    // allocate: fill cache on miss
    bool accessLevel(CacheLevel& level, size_t physical_address, CacheAccessReport& report, bool update_stats = true, bool allocate = true);
    
    // Address calculation
    size_t extractTag(CacheLevel& level, size_t address) const;
    size_t extractSetIndex(CacheLevel& level, size_t address) const;
    size_t extractBlockOffset(CacheLevel& level, size_t address) const;
    
    // Replacement policies
    size_t findVictimFIFO(CacheSet& set);
    size_t findVictimLRU(CacheSet& set);
    size_t findVictimLFU(CacheSet& set);
    
    void updateReplacementData(CacheLevel& level, CacheSet& set, size_t block_index, ReplacementPolicy policy);
};

#endif // CACHE_SIMULATOR_H
