#include "CacheSimulator.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <sstream> // Added for stringstream

CacheSimulator::CacheSimulator(size_t l1_size, size_t l1_block_size, size_t l1_associativity,
                               size_t l2_size, size_t l2_block_size, size_t l2_associativity,
                               ReplacementPolicy policy)
    : defaultPolicy(policy) {
    initCacheLevel(l1_cache, 1, l1_size, l1_block_size, l1_associativity, policy);
    initCacheLevel(l2_cache, 2, l2_size, l2_block_size, l2_associativity, policy);
}

void CacheSimulator::initCacheLevel(CacheLevel& level, int levelNum, size_t size, size_t block_size, 
                                   size_t associativity, ReplacementPolicy policy) {
    level.levelNum = levelNum;
    level.size = size;
    level.block_size = block_size;
    level.associativity = associativity;
    level.policy = policy;
    level.hits = 0;
    level.misses = 0;
    level.evictions = 0;
    level.global_time = 0;
    
    // Calculate cache parameters
    level.num_sets = size / (block_size * associativity);
    level.block_offset_bits = static_cast<size_t>(std::log2(block_size));
    level.set_index_bits = static_cast<size_t>(std::log2(level.num_sets));
    level.tag_bits = 64 - level.set_index_bits - level.block_offset_bits; // Assuming 64-bit addresses
    
    // Initialize sets
    level.sets.resize(level.num_sets);
    for (auto& set : level.sets) {
        set.blocks.resize(associativity);
        for (auto& block : set.blocks) {
            block.valid = false;
            block.tag = 0;
            block.load_time = 0;
            block.last_access = 0;
            block.access_count = 0;
        }
        set.associativity = associativity;
    }
}

CacheSimulator::CacheAccessReport CacheSimulator::access(size_t physical_address) {
    CacheAccessReport report;
    report.l1Hit = false;
    report.l2Hit = false;
    report.l2Accessed = false;

    // Try L1 first (Probe only)
    report.l1Hit = accessLevel(l1_cache, physical_address, report, true, false);
    
    if (report.l1Hit) {
        return report; // L1 hit
    }
    
    // L1 miss, try L2 (Probe only)
    report.l2Accessed = true;
    report.l2Hit = accessLevel(l2_cache, physical_address, report, true, false);
    
    if (report.l2Hit) {
        // Load into L1 (may evict from L1)
        accessLevel(l1_cache, physical_address, report, false, true);
        return report; // Overall miss (L1 miss, L2 hit)
    }
    
    // Both miss - data must be loaded from memory
    // Load into L2 first, then L1
    accessLevel(l2_cache, physical_address, report, false, true);
    accessLevel(l1_cache, physical_address, report, false, true);
    
    return report; // Overall miss
}

bool CacheSimulator::accessLevel(CacheLevel& level, size_t physical_address, CacheAccessReport& report, bool update_stats, bool allocate) {
    if (update_stats) level.global_time++;
    
    size_t tag = extractTag(level, physical_address);
    size_t set_index = extractSetIndex(level, physical_address);
    
    if (set_index >= level.sets.size()) {
        return false;
    }
    
    CacheSet& set = level.sets[set_index];
    
    // Check if block is in cache
    for (size_t i = 0; i < set.blocks.size(); i++) {
        if (set.blocks[i].valid && set.blocks[i].tag == tag) {
            // Cache hit
            if (update_stats) {
                level.hits++;
                updateReplacementData(level, set, i, level.policy);
            } else {
                 // Even on fill (if we call it redundantly) or just silent probe?
                 // If allocate=true and hit, we just found it.
                 // If update_stats=false, we probably don't want to mess with replacement unless it's a "use".
                 // Let's assume updateReplacementData is safe.
            }
            return true;
        }
    }
    
    // Cache miss
    if (update_stats) {
        level.misses++;
    }
    
    if (!allocate) {
        return false;
    }
    
    // Find a slot for the new block
    size_t victim_index = set.associativity;
    for (size_t i = 0; i < set.blocks.size(); i++) {
        if (!set.blocks[i].valid) {
            victim_index = i;
            break;
        }
    }
    
    // If cache is full, find victim using replacement policy
    if (victim_index == set.associativity) {
        switch (level.policy) {
            case FIFO:
                victim_index = findVictimFIFO(set);
                break;
            case LRU:
                victim_index = findVictimLRU(set);
                break;
            case LFU:
                victim_index = findVictimLFU(set);
                break;
        }
        level.evictions++; // Eviction always happens on allocation if full
        
        // Log eviction
        std::stringstream ss;
        ss << "L" << level.levelNum << " Eviction: Tag 0x" << std::hex << set.blocks[victim_index].tag << std::dec 
           << " (Set " << set_index << ")";
        report.events.push_back(ss.str());
    }
    
    // Load new block
    set.blocks[victim_index].valid = true;
    set.blocks[victim_index].tag = tag;
    // Should we update time if not updating stats? 
    // Yes, a fill sets the load time.
    size_t current_time = level.global_time; // If not updated above, uses old time?
    // If update_stats is false, global_time wasn't incremented.
    // It's fine, simulated simultaneity.
    
    set.blocks[victim_index].load_time = current_time;
    set.blocks[victim_index].last_access = current_time;
    set.blocks[victim_index].access_count = 1;
    
    // Update replacement data structures
    // Correctly initialize FIFO queue or LRU list for the new victim
    // The helper updateReplacementData might expect the block to be in the set?
    // For FIFO: findVictimFIFO popped the victim. We need to push the new one?
    // findVictimFIFO: pops front, pushes back. 
    // Wait, findVictimFIFO pushes the VICTIM back to end? 
    // "set.fifoQueue.push(victim); // Re-insert at end" -> This rotates the queue.
    // If we overwrite block[victim], we effectively reused the slot.
    // The slot ID is effectively "new" now.
    // Actually, queue stores INDICES.
    // If we replace block at index X. Index X moves to back of queue?
    // FIFO: First In (Oldest) is victim.
    // If we use index X, X is now Newest. So yes, back of queue.
    // `findVictimFIFO` implementation I saw:
    // val = front; pop; push(val); return val;
    // This moves the victim index to the back. Correct for "Reuse slot X as new".
    
    updateReplacementData(level, set, victim_index, level.policy);
    
    return false; // Miss (but loaded)
}

size_t CacheSimulator::extractTag(CacheLevel& level, size_t address) const {
    size_t mask = (1ULL << level.tag_bits) - 1;
    return (address >> (level.set_index_bits + level.block_offset_bits)) & mask;
}

size_t CacheSimulator::extractSetIndex(CacheLevel& level, size_t address) const {
    size_t mask = (1ULL << level.set_index_bits) - 1;
    return (address >> level.block_offset_bits) & mask;
}

size_t CacheSimulator::extractBlockOffset(CacheLevel& level, size_t address) const {
    size_t mask = (1ULL << level.block_offset_bits) - 1;
    return address & mask;
}

size_t CacheSimulator::findVictimFIFO(CacheSet& set) {
    if (set.fifoQueue.empty()) {
        // Initialize queue
        for (size_t i = 0; i < set.blocks.size(); i++) {
            set.fifoQueue.push(i);
        }
    }
    
    size_t victim = set.fifoQueue.front();
    set.fifoQueue.pop();
    set.fifoQueue.push(victim); // Re-insert at end
    
    return victim;
}

size_t CacheSimulator::findVictimLRU(CacheSet& set) {
    // Find block with oldest last_access time
    size_t victim = 0;
    size_t oldest_time = set.blocks[0].last_access;
    
    for (size_t i = 1; i < set.blocks.size(); i++) {
        if (set.blocks[i].last_access < oldest_time) {
            oldest_time = set.blocks[i].last_access;
            victim = i;
        }
    }
    
    // Update LRU list
    set.lruList.remove(victim);
    set.lruList.push_back(victim);
    
    return victim;
}

size_t CacheSimulator::findVictimLFU(CacheSet& set) {
    // Find block with lowest access count
    size_t victim = 0;
    size_t min_count = set.blocks[0].access_count;
    
    for (size_t i = 1; i < set.blocks.size(); i++) {
        if (set.blocks[i].access_count < min_count) {
            min_count = set.blocks[i].access_count;
            victim = i;
        }
    }
    
    return victim;
}

void CacheSimulator::updateReplacementData(CacheLevel& level, CacheSet& set, 
                                          size_t block_index, ReplacementPolicy policy) {
    switch (policy) {
        case FIFO:
            // FIFO doesn't need updates on access, only on insertion
            break;
        case LRU:
            // Move to end of LRU list (most recently used)
            set.lruList.remove(block_index);
            set.lruList.push_back(block_index);
            set.blocks[block_index].last_access = level.global_time;
            break;
        case LFU:
            // Increment access count
            set.blocks[block_index].access_count++;
            break;
    }
}

void CacheSimulator::setReplacementPolicy(ReplacementPolicy policy) {
    defaultPolicy = policy;
    l1_cache.policy = policy;
    l2_cache.policy = policy;
}

void CacheSimulator::setReplacementPolicy(size_t level, ReplacementPolicy policy) {
    if (level == 1) {
        l1_cache.policy = policy;
    } else if (level == 2) {
        l2_cache.policy = policy;
    }
}

size_t CacheSimulator::getHits(size_t level) const {
    if (level == 1) return l1_cache.hits;
    if (level == 2) return l2_cache.hits;
    return 0;
}

size_t CacheSimulator::getMisses(size_t level) const {
    if (level == 1) return l1_cache.misses;
    if (level == 2) return l2_cache.misses;
    return 0;
}

double CacheSimulator::getHitRatio(size_t level) const {
    size_t hits, misses;
    if (level == 1) {
        hits = l1_cache.hits;
        misses = l1_cache.misses;
    } else if (level == 2) {
        hits = l2_cache.hits;
        misses = l2_cache.misses;
    } else {
        return 0.0;
    }
    
    size_t total = hits + misses;
    if (total == 0) return 0.0;
    return (static_cast<double>(hits) / total) * 100.0;
}

void CacheSimulator::printStatistics() const {
    std::cout << "\n=== Cache Statistics ===\n";
    
    // Constants for AMAT calculation (assumptions)
    const double L1_LATENCY = 1.0;
    const double L2_LATENCY = 10.0;
    const double MEM_LATENCY = 100.0;
    
    double l1_mr = 1.0 - (getHitRatio(1) / 100.0);
    double l2_mr = 1.0 - (getHitRatio(2) / 100.0);
    // If no accesses, miss rate is 0 to avoid skewing, or 1? Let's say 0 for AMAT if empty.
    // Actually if hits+misses=0, MR is meaningless.
    
    size_t l1_total = l1_cache.hits + l1_cache.misses;
    size_t l2_total = l2_cache.hits + l2_cache.misses;
    
    if (l1_total > 0) l1_mr = (double)l1_cache.misses / l1_total; else l1_mr = 0;
    if (l2_total > 0) l2_mr = (double)l2_cache.misses / l2_total; else l2_mr = 0;

    double amat = L1_LATENCY + l1_mr * (L2_LATENCY + l2_mr * MEM_LATENCY);

    std::cout << "L1 Cache:\n";
    std::cout << "  Hits: " << l1_cache.hits << "\n";
    std::cout << "  Misses: " << l1_cache.misses << "\n";
    std::cout << "  Evictions: " << l1_cache.evictions << "\n";
    std::cout << "  Hit Ratio: " << std::fixed << std::setprecision(2) 
              << getHitRatio(1) << "%\n";
    std::cout << "  Miss Traffic (to L2): " << l1_cache.misses << " requests\n";
    
    std::cout << "L2 Cache:\n";
    std::cout << "  Hits: " << l2_cache.hits << "\n";
    std::cout << "  Misses: " << l2_cache.misses << "\n";
    std::cout << "  Evictions: " << l2_cache.evictions << "\n";
    std::cout << "  Hit Ratio: " << std::fixed << std::setprecision(2) 
              << getHitRatio(2) << "%\n";
    std::cout << "  Miss Traffic (to Memory): " << l2_cache.misses << " requests\n";
    
    std::cout << "System Performance:\n";
    std::cout << "  Estimated AMAT: " << amat << " cycles\n";
    std::cout << "  (Assumptions: L1=" << (int)L1_LATENCY << ", L2=" << (int)L2_LATENCY 
              << ", Mem=" << (int)MEM_LATENCY << ")\n";
    
    std::cout << "======================\n\n";
}
