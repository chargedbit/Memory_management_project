#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <cstddef>
#include <vector>
#include <list>
#include <map>
#include <cstdint>

class MemoryManager {
public:
    enum AllocationStrategy {
        FIRST_FIT,
        BEST_FIT,
        WORST_FIT
    };

    struct BlockInfo {
        size_t block_id;
        void* address;
        size_t size;
        bool is_free;
    };

    MemoryManager(size_t totalSize, AllocationStrategy strategy = FIRST_FIT);
    ~MemoryManager();
    
    // Allocation interface
    void* allocate(size_t size);
    bool deallocate(void* ptr);
    bool deallocate(size_t block_id);
    void setAllocationStrategy(AllocationStrategy strategy);
    
    // Statistics and information
    void dumpMemory() const;
    double getInternalFragmentation() const;
    double getExternalFragmentation() const;
    double getMemoryUtilization() const;
    size_t getTotalMemory() const { return totalMemorySize; }
    size_t getUsedMemory() const;
    size_t getFreeMemory() const;
    size_t getAllocationSuccessCount() const { return allocationSuccessCount; }
    size_t getAllocationFailureCount() const { return allocationFailureCount; }
    
    // Get block information
    BlockInfo getBlockInfo(void* ptr) const;
    std::vector<BlockInfo> getAllBlocks() const;

private:
    // Block header structure (embedded in memory)
    struct BlockHeader {
        size_t size;              // Size of this block (including header)
        bool is_free;             // Allocation status
        size_t block_id;           // Unique block identifier
        BlockHeader* next;         // Next block in memory
        BlockHeader* prev;         // Previous block in memory
    };

    size_t totalMemorySize;
    AllocationStrategy currentStrategy;
    std::vector<char> physicalMemory;
    
    BlockHeader* firstBlock;      // First block in memory
    BlockHeader* freeListHead;    // Head of free list (for First/Best/Worst Fit)
    
    // Block tracking
    size_t nextBlockId;
    std::map<void*, BlockHeader*> addressToHeader;
    std::map<size_t, BlockHeader*> idToHeader;
    std::map<size_t, size_t> idToRequestedSize;  // Track requested size per block
    
    // Statistics
    size_t allocationSuccessCount;
    size_t allocationFailureCount;
    size_t totalRequestedSize;
    size_t totalAllocatedSize;

    // Helper functions
    BlockHeader* findFirstFit(size_t size);
    BlockHeader* findBestFit(size_t size);
    BlockHeader* findWorstFit(size_t size);
    
    void splitBlock(BlockHeader* block, size_t requestedSize);
    void coalesceBlocks(BlockHeader* block);
    
    void addToFreeList(BlockHeader* block);
    void removeFromFreeList(BlockHeader* block);
    
    // Initialization
    void initializeMemory();
    
    // Utility
    bool isValidPointer(void* ptr) const;
    BlockHeader* getHeader(void* ptr) const;
    size_t getLargestFreeBlock() const;
};

#endif // MEMORY_MANAGER_H
