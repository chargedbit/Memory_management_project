#include "MemoryManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

MemoryManager::MemoryManager(size_t totalSize, AllocationStrategy strategy)
    : totalMemorySize(totalSize), currentStrategy(strategy),
      physicalMemory(totalSize, 0), firstBlock(nullptr), freeListHead(nullptr),
      nextBlockId(1), allocationSuccessCount(0), allocationFailureCount(0),
      totalRequestedSize(0), totalAllocatedSize(0) {
    initializeMemory();
}

MemoryManager::~MemoryManager() {
    // Memory is managed by vector, no explicit cleanup needed
}

void MemoryManager::initializeMemory() {
    // Create initial free block covering entire memory
    firstBlock = reinterpret_cast<BlockHeader*>(physicalMemory.data());
    firstBlock->size = totalMemorySize;
    firstBlock->is_free = true;
    firstBlock->block_id = 0; // Special ID for initial block
    firstBlock->next = nullptr;
    firstBlock->prev = nullptr;
    
    freeListHead = firstBlock;
    addressToHeader[reinterpret_cast<void*>(firstBlock)] = firstBlock;
}

void* MemoryManager::allocate(size_t size) {
    if (size == 0) {
        allocationFailureCount++;
        return nullptr;
    }
    
    // Add header size to requested size
    size_t requiredSize = size + sizeof(BlockHeader);
    totalRequestedSize += size;
    
    BlockHeader* block = nullptr;
    
    switch (currentStrategy) {
        case FIRST_FIT:
            block = findFirstFit(requiredSize);
            break;
        case BEST_FIT:
            block = findBestFit(requiredSize);
            break;
        case WORST_FIT:
            block = findWorstFit(requiredSize);
            break;
    }
    
    if (block == nullptr) {
        allocationFailureCount++;
        return nullptr;
    }
    
    // Split block if it's large enough to create another block
    if (block->size >= requiredSize + sizeof(BlockHeader) + 8) {
        splitBlock(block, requiredSize);
    }
    
    // Mark as allocated
    block->is_free = false;
    block->block_id = nextBlockId++;
    
    // Remove from free list
    removeFromFreeList(block);
    
    // Track allocation
    void* userPtr = reinterpret_cast<char*>(block) + sizeof(BlockHeader);
    addressToHeader[userPtr] = block;
    idToHeader[block->block_id] = block;
    idToRequestedSize[block->block_id] = size;  // Store requested size
    
    totalAllocatedSize += (block->size - sizeof(BlockHeader));
    allocationSuccessCount++;
    
    return userPtr;
}

bool MemoryManager::deallocate(void* ptr) {
    if (!isValidPointer(ptr)) {
        return false;
    }
    
    BlockHeader* block = getHeader(ptr);
    if (block == nullptr || block->is_free) {
        return false;
    }
    
    // Mark as free
    block->is_free = true;
    totalAllocatedSize -= (block->size - sizeof(BlockHeader));
    
    // Remove requested size tracking
    idToRequestedSize.erase(block->block_id);
    
    // Add to free list
    addToFreeList(block);
    coalesceBlocks(block);
    
    // Remove from tracking
    addressToHeader.erase(ptr);
    idToHeader.erase(block->block_id);
    
    return true;
}

bool MemoryManager::deallocate(size_t block_id) {
    auto it = idToHeader.find(block_id);
    if (it == idToHeader.end()) {
        return false;
    }
    
    BlockHeader* block = it->second;
    void* userPtr = reinterpret_cast<char*>(block) + sizeof(BlockHeader);
    return deallocate(userPtr);
}

void MemoryManager::setAllocationStrategy(AllocationStrategy strategy) {
    if (strategy == currentStrategy) {
        return;
    }
    
    // Rebuild free lists for new strategy (Just reset strategy variable for list-based strategies)
    // Actually, all current strategies (First, Best, Worst) share the same freeList structure/logic.
    // They just traverse it differently. So no need to rebuild free list unless we had separate lists.
    // In this implementation, `freeListHead` seems to be a single list.
    
    currentStrategy = strategy;
}

// First Fit: Find first block that fits
MemoryManager::BlockHeader* MemoryManager::findFirstFit(size_t size) {
    BlockHeader* current = freeListHead;
    while (current != nullptr) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Best Fit: Find smallest block that fits
MemoryManager::BlockHeader* MemoryManager::findBestFit(size_t size) {
    BlockHeader* best = nullptr;
    BlockHeader* current = freeListHead;
    size_t iterations = 0;
    const size_t maxIterations = 10000; // Safety limit to prevent infinite loops
    
    while (current != nullptr && iterations < maxIterations) {
        if (current->is_free && current->size >= size) {
            if (best == nullptr || current->size < best->size) {
                best = current;
            }
        }
        current = current->next;
        iterations++;
    }
    
    return best;
}

// Worst Fit: Find largest block that fits
MemoryManager::BlockHeader* MemoryManager::findWorstFit(size_t size) {
    BlockHeader* worst = nullptr;
    BlockHeader* current = freeListHead;
    
    while (current != nullptr) {
        if (current->is_free && current->size >= size) {
            if (worst == nullptr || current->size > worst->size) {
                worst = current;
            }
        }
        current = current->next;
    }
    
    return worst;
}



void MemoryManager::splitBlock(BlockHeader* block, size_t requestedSize) {
    size_t remainingSize = block->size - requestedSize;
    // Need at least sizeof(BlockHeader) to create a new block
    // Add a small threshold (8 bytes) to avoid creating blocks with very little usable space
    if (remainingSize < sizeof(BlockHeader) + 8) { // Too small to split
        return;
    }
    
    // Create new free block from remaining space
    BlockHeader* newBlock = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<char*>(block) + requestedSize);
    newBlock->size = remainingSize;
    newBlock->is_free = true;
    newBlock->block_id = 0;
    // Don't set prev/next here - they're used for free list, not physical order
    // Physical order is determined by address arithmetic
    
    block->size = requestedSize;
    
    // Add new block to free list (this will set its next/prev for free list)
    addToFreeList(newBlock);
}

void MemoryManager::coalesceBlocks(BlockHeader* block) {
    // Try to merge with next block in physical memory
    char* blockEnd = reinterpret_cast<char*>(block) + block->size;
    if (blockEnd < physicalMemory.data() + totalMemorySize) {
        BlockHeader* next = reinterpret_cast<BlockHeader*>(blockEnd);
        if (next->is_free) {
            removeFromFreeList(next);
            block->size += next->size;
        }
    }
    
    // Try to merge with previous block in physical memory
    // Find the block that ends just before this block starts
    BlockHeader* current = firstBlock;
    char* blockStart = reinterpret_cast<char*>(block);
    BlockHeader* prev = nullptr;
    size_t iterations = 0;
    const size_t maxIterations = 10000; // Safety limit
    
    while (current != nullptr && current != block && iterations < maxIterations) {
        char* currentEnd = reinterpret_cast<char*>(current) + current->size;
        if (currentEnd == blockStart) {
            prev = current;
            break;
        }
        // Move to next block in physical memory
        if (currentEnd < physicalMemory.data() + totalMemorySize) {
            current = reinterpret_cast<BlockHeader*>(currentEnd);
        } else {
            break;
        }
        iterations++;
    }
    
    if (prev != nullptr && prev->is_free) {
        removeFromFreeList(block);
        prev->size += block->size;
        // Recursively try to coalesce the merged block
        coalesceBlocks(prev);
    }
}

void MemoryManager::addToFreeList(BlockHeader* block) {
    // Safety check: if block is already in the free list, remove it first
    // This prevents cycles and corruption
    if (block->prev != nullptr || block->next != nullptr || block == freeListHead) {
        removeFromFreeList(block);
    }
    
    block->next = freeListHead;
    block->prev = nullptr;
    if (freeListHead != nullptr) {
        freeListHead->prev = block;
    }
    freeListHead = block;
}

void MemoryManager::removeFromFreeList(BlockHeader* block) {
    if (block->prev != nullptr) {
        block->prev->next = block->next;
    } else {
        freeListHead = block->next;
    }
    if (block->next != nullptr) {
        block->next->prev = block->prev;
    }
    block->next = nullptr;
    block->prev = nullptr;
}



bool MemoryManager::isValidPointer(void* ptr) const {
    if (ptr == nullptr) return false;
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    uintptr_t base = reinterpret_cast<uintptr_t>(physicalMemory.data());
    uintptr_t end = base + totalMemorySize;
    return addr >= base && addr < end;
}

MemoryManager::BlockHeader* MemoryManager::getHeader(void* ptr) const {
    auto it = addressToHeader.find(ptr);
    if (it != addressToHeader.end()) {
        return it->second;
    }
    return nullptr;
}

size_t MemoryManager::getLargestFreeBlock() const {
    size_t largest = 0;
    
    // Iterate through physical memory instead of free list to avoid potential cycles
    const BlockHeader* current = firstBlock;
    size_t address = 0;
    size_t maxIterations = 1000;  // Safety limit
    size_t iterations = 0;
    
    while (current != nullptr && address < totalMemorySize && iterations < maxIterations) {
        if (current->is_free && current->size > largest) {
            largest = current->size;
        }
        
        address += current->size;
        iterations++;
        
        // Move to next block
        if (address < totalMemorySize) {
            current = reinterpret_cast<const BlockHeader*>(
                reinterpret_cast<const char*>(physicalMemory.data()) + address);
        } else {
            break;
        }
    }
    
    return largest;
}

double MemoryManager::getInternalFragmentation() const {
    // Calculate internal fragmentation by iterating through allocated blocks
    size_t totalAllocated = 0;
    size_t totalRequested = 0;
    
    for (const auto& pair : idToHeader) {
        const BlockHeader* block = pair.second;
        if (!block->is_free) {
            size_t usableSize = block->size - sizeof(BlockHeader);
            totalAllocated += usableSize;
            
            // Get requested size for this block
            auto it = idToRequestedSize.find(block->block_id);
            if (it != idToRequestedSize.end()) {
                totalRequested += it->second;
            }
        }
    }
    
    if (totalAllocated == 0) return 0.0;
    
    size_t wasted = totalAllocated - totalRequested;
    return (static_cast<double>(wasted) / totalAllocated) * 100.0;
}

double MemoryManager::getExternalFragmentation() const {
    if (totalMemorySize == 0) return 0.0;
    
    // Calculate total free memory (usable space in free blocks)
    size_t totalFreeUsable = 0;
    size_t largestFreeUsable = 0;
    
    // Iterate through physical memory to find free blocks
    const BlockHeader* current = firstBlock;
    size_t address = 0;
    size_t maxIterations = 1000;
    size_t iterations = 0;
    
    while (current != nullptr && address < totalMemorySize && iterations < maxIterations) {
        if (current->is_free) {
            size_t usableSize = current->size - sizeof(BlockHeader);
            totalFreeUsable += usableSize;
            if (usableSize > largestFreeUsable) {
                largestFreeUsable = usableSize;
            }
        }
        
        address += current->size;
        iterations++;
        
        if (address < totalMemorySize) {
            current = reinterpret_cast<const BlockHeader*>(
                reinterpret_cast<const char*>(physicalMemory.data()) + address);
        } else {
            break;
        }
    }
    
    if (totalFreeUsable == 0) return 0.0;
    
    // External fragmentation = (total free - largest free) / total memory
    size_t externalFrag = (totalFreeUsable > largestFreeUsable) ? 
                          (totalFreeUsable - largestFreeUsable) : 0;
    return (static_cast<double>(externalFrag) / totalMemorySize) * 100.0;
}

double MemoryManager::getMemoryUtilization() const {
    if (totalMemorySize == 0) return 0.0;
    return (static_cast<double>(getUsedMemory()) / totalMemorySize) * 100.0;
}

size_t MemoryManager::getUsedMemory() const {
    size_t used = 0;
    
    // Iterate through all allocated blocks and sum their actual sizes (including headers)
    for (const auto& pair : idToHeader) {
        const BlockHeader* block = pair.second;
        if (!block->is_free) {
            used += block->size;  // Block size includes the header
        }
    }
    
    return used;
}

size_t MemoryManager::getFreeMemory() const {
    size_t used = getUsedMemory();
    if (used > totalMemorySize) {
        return 0;  // Safety check
    }
    return totalMemorySize - used;
}

void MemoryManager::dumpMemory() const {
    std::cout << "\n=== Memory Dump ===\n";
    
    // Iterate through physical memory
    const BlockHeader* current = firstBlock;
    size_t address = 0;
    size_t maxIterations = 1000;
    size_t iterations = 0;
    
    while (current != nullptr && address < totalMemorySize && iterations < maxIterations) {
        size_t blockSize = current->size;
        
        std::cout << std::hex << std::setfill('0');
        std::cout << "[0x" << std::setw(8) << address << " - 0x" 
                  << std::setw(8) << (address + blockSize - 1) << "] ";
        std::cout << std::dec;
        
        if (current->is_free) {
            std::cout << "FREE";
        } else {
            std::cout << "USED (id=" << current->block_id << ", size=" 
                      << (blockSize - sizeof(BlockHeader)) << " bytes)";
        }
        std::cout << "\n";
        
        address += blockSize;
        iterations++;
        
        // Move to next block
        if (address < totalMemorySize) {
            current = reinterpret_cast<const BlockHeader*>(
                reinterpret_cast<const char*>(physicalMemory.data()) + address);
        } else {
            break;
        }
    }
    std::cout << "==================\n\n";
}

MemoryManager::BlockInfo MemoryManager::getBlockInfo(void* ptr) const {
    BlockInfo info;
    BlockHeader* header = getHeader(ptr);
    if (header != nullptr) {
        info.block_id = header->block_id;
        info.address = ptr;
        info.size = header->size - sizeof(BlockHeader);
        info.is_free = header->is_free;
    }
    return info;
}

std::vector<MemoryManager::BlockInfo> MemoryManager::getAllBlocks() const {
    std::vector<BlockInfo> blocks;
    for (const auto& pair : idToHeader) {
        BlockHeader* header = pair.second;
        BlockInfo info;
        info.block_id = header->block_id;
        info.address = reinterpret_cast<void*>(
            reinterpret_cast<char*>(header) + sizeof(BlockHeader));
        info.size = header->size - sizeof(BlockHeader);
        info.is_free = header->is_free;
        blocks.push_back(info);
    }
    return blocks;
}
