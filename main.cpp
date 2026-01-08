#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <map>
#include "allocator/MemoryManager.h"
#include "cache/CacheSimulator.h"
#include "stats/StatsManager.h"

class MemorySimulatorCLI {
private:
    MemoryManager* memoryManager;
    CacheSimulator* cacheSimulator;
    StatsManager* statsManager;
    
    bool initialized;
    size_t nextBlockId;
    std::map<size_t, void*> blockIdToAddress;
    std::map<void*, size_t> addressToBlockId;

public:
    MemorySimulatorCLI() 
        : memoryManager(nullptr), cacheSimulator(nullptr), statsManager(new StatsManager()), 
          initialized(false), nextBlockId(1) {}
    
    ~MemorySimulatorCLI() {
        delete memoryManager;
        delete cacheSimulator;
        delete statsManager;
    }
    
    void run() {
        std::cout << "Memory Management Simulator\n";
        std::cout << "Type 'help' for available commands\n\n";
        
        std::string line;
        while (true) {
            std::cout << "> ";
            std::getline(std::cin, line);
            
            if (line.empty()) continue;
            
            std::vector<std::string> tokens = tokenize(line);
            if (tokens.empty()) continue;
            
            std::string command = tokens[0];
            std::transform(command.begin(), command.end(), command.begin(), ::tolower);
            
            if (command == "exit" || command == "quit") {
                break;
            } else if (command == "help") {
                printHelp();
            } else if (command == "init") {
                handleInit(tokens);
            } else if (command == "set") {
                handleSet(tokens);
            } else if (command == "malloc") {
                handleMalloc(tokens);
            } else if (command == "free") {
                handleFree(tokens);
            } else if (command == "dump") {
                handleDump(tokens);
            } else if (command == "stats") {
                handleStats();
            } else if (command == "access") {
                handleAccess(tokens);
            } else {
                std::cout << "Unknown command: " << command << "\n";
                std::cout << "Type 'help' for available commands\n";
            }
        }
        
        std::cout << "Simulator exited.\n";
    }
    
private:
    std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream iss(line);
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
    void printHelp() {
        std::cout << "\nAvailable commands:\n";
        std::cout << "  init memory <size>            - Initialize memory system (RAM + Cache)\n";
        std::cout << "  init cache <params...>        - Initialize L1/L2 cache hierarchy\n";
        std::cout << "  set allocator <strategy>      - Set allocation strategy (first_fit, best_fit, worst_fit)\n";
        std::cout << "  set cache_policy <policy>     - Set cache replacement policy (fifo, lru, lfu)\n";
        std::cout << "  malloc <size>                 - Allocate memory block\n";

        std::cout << "  free <block_id>               - Free memory block by ID\n";
        std::cout << "  free 0x<address>              - Free memory block by address\n";
        std::cout << "  dump memory                   - Display memory layout\n";
        std::cout << "  stats                         - Display statistics\n";
        std::cout << "  access <address>              - Simulate cache access (Physical Address)\n";
        std::cout << "  help                          - Show this help\n";
        std::cout << "  exit                          - Exit simulator\n\n";
    }
    
    void handleInit(const std::vector<std::string>& tokens) {
        if (tokens.size() < 2) {
             std::cout << "Usage: init memory <size> OR init cache <params>\n";
             return;
        }

        if (tokens[1] == "memory") {
            if (tokens.size() < 3) {
                std::cout << "Usage: init memory <size>\n";
                return;
            }
            
            size_t size = std::stoull(tokens[2]);
            
            // Delete existing managers
            delete memoryManager;
            memoryManager = nullptr;
            
            // Initialize memory manager
            memoryManager = new MemoryManager(size, MemoryManager::FIRST_FIT);
            
            if (cacheSimulator == nullptr) {
                // Initialize cache simulator (default sizes)
                cacheSimulator = new CacheSimulator(
                    16 * 1024, 64, 4,   // L1: 16KB, 64B block, 4-way
                    64 * 1024, 64, 8    // L2: 64KB, 64B block, 8-way
                );
            }
            
            initialized = true;
            nextBlockId = 1;
            blockIdToAddress.clear();
            addressToBlockId.clear();
            
            std::cout << "Memory initialized with size: " << size << " bytes\n";

        } else if (tokens[1] == "cache") {
            // init cache <l1_size> <l1_block_size> <l1_assoc> <l2_size> <l2_block_size> <l2_assoc>
            if (tokens.size() < 8) {
                std::cout << "Usage: init cache <l1_sz> <l1_blk> <l1_assoc> <l2_sz> <l2_blk> <l2_assoc>\n";
                return;
            }
            
            try {
                size_t l1_size = std::stoull(tokens[2]);
                size_t l1_block = std::stoull(tokens[3]);
                size_t l1_assoc = std::stoull(tokens[4]);
                size_t l2_size = std::stoull(tokens[5]);
                size_t l2_block = std::stoull(tokens[6]);
                size_t l2_assoc = std::stoull(tokens[7]);
                
                delete cacheSimulator;
                cacheSimulator = new CacheSimulator(
                    l1_size, l1_block, l1_assoc,
                    l2_size, l2_block, l2_assoc
                );
                
                std::cout << "Cache initialized:\n";
                std::cout << "L1: " << l1_size << "B, " << l1_block << "B blocks, " << l1_assoc << "-way\n";
                std::cout << "L2: " << l2_size << "B, " << l2_block << "B blocks, " << l2_assoc << "-way\n";
                
            } catch (const std::exception& e) {
                std::cout << "Error parsing cache parameters: " << e.what() << "\n";
            }
        } else {
             std::cout << "Unknown init subcommand: " << tokens[1] << "\n";
        }
    }
    
    void handleSet(const std::vector<std::string>& tokens) {
        if (!initialized) {
            std::cout << "Error: Memory not initialized. Use 'init memory <size>' first.\n";
            return;
        }
        
        if (tokens.size() >= 3 && tokens[1] == "cache_policy") {
            // set cache_policy <fifo|lru|lfu>
            std::string policyName = tokens[2];
            std::transform(policyName.begin(), policyName.end(), policyName.begin(), ::tolower);
            
            CacheSimulator::ReplacementPolicy policy;
            if (policyName == "fifo") {
                policy = CacheSimulator::FIFO;
            } else if (policyName == "lru") {
                policy = CacheSimulator::LRU;
            } else if (policyName == "lfu") {
                policy = CacheSimulator::LFU;
            } else {
                std::cout << "Invalid policy. Use: fifo, lru, or lfu\n";
                return;
            }
            
            if (cacheSimulator) {
                cacheSimulator->setReplacementPolicy(policy);
                std::cout << "Cache replacement policy set to: " << policyName << "\n";
            } else {
                std::cout << "Cache not initialized. Use init init memory or init cache first.\n";
            }
            return;
        }

        if (tokens.size() < 3 || tokens[1] != "allocator") {
            std::cout << "Usage: set allocator <strategy> OR set cache_policy <policy>\n";
            std::cout << "Strategies: first_fit, best_fit, worst_fit\n";
            std::cout << "Policies: fifo, lru, lfu\n";
            return;
        }
        
        std::string strategy = tokens[2];
        std::transform(strategy.begin(), strategy.end(), strategy.begin(), ::tolower);
        
        MemoryManager::AllocationStrategy allocStrategy;
        if (strategy == "first_fit" || strategy == "firstfit") {
            allocStrategy = MemoryManager::FIRST_FIT;
        } else if (strategy == "best_fit" || strategy == "bestfit") {
            allocStrategy = MemoryManager::BEST_FIT;
        } else if (strategy == "worst_fit" || strategy == "worstfit") {
            allocStrategy = MemoryManager::WORST_FIT;
        } else {
            std::cout << "Invalid strategy. Use: first_fit, best_fit, worst_fit\n";
            return;
        }
        
        memoryManager->setAllocationStrategy(allocStrategy);
        std::cout << "Allocation strategy set to: " << strategy << "\n";
    }
    
    void handleMalloc(const std::vector<std::string>& tokens) {
        if (!initialized) {
            std::cout << "Error: Memory not initialized. Use 'init memory <size>' first.\n";
            return;
        }
        
        if (tokens.size() < 2) {
            std::cout << "Usage: malloc <size>\n";
            return;
        }
        
        size_t size = std::stoull(tokens[1]);
        void* ptr = memoryManager->allocate(size);
        
        if (ptr != nullptr) {
            size_t blockId = nextBlockId++;
            blockIdToAddress[blockId] = ptr;
            addressToBlockId[ptr] = blockId;
            
            statsManager->logMemoryAllocation(size, true);
            std::cout << "Allocated block id=" << blockId << " at address=0x" 
                      << std::hex << reinterpret_cast<uintptr_t>(ptr) << std::dec << "\n";
        } else {
            statsManager->logMemoryAllocation(size, false);
            std::cout << "Failed to allocate " << size << " bytes\n";
        }
    }
    
    void handleFree(const std::vector<std::string>& tokens) {
        if (!initialized) {
            std::cout << "Error: Memory not initialized.\n";
            return;
        }
        
        if (tokens.size() < 2) {
            std::cout << "Usage: free <block_id> or free 0x<address>\n";
            return;
        }
        
        std::string arg = tokens[1];
        bool success = false;
        
        // Check if it's a hex address
        if (arg.substr(0, 2) == "0x" || arg.substr(0, 2) == "0X") {
            uintptr_t addr = std::stoull(arg, nullptr, 16);
            void* ptr = reinterpret_cast<void*>(addr);
            
            auto it = addressToBlockId.find(ptr);
            if (it != addressToBlockId.end()) {
                success = memoryManager->deallocate(ptr);
                if (success) {
                    size_t blockId = it->second;
                    blockIdToAddress.erase(blockId);
                    addressToBlockId.erase(ptr);
                    std::cout << "Block " << blockId << " freed and merged\n";
                }
            } else {
                success = memoryManager->deallocate(ptr);
                if (success) {
                    std::cout << "Address 0x" << std::hex << addr << std::dec << " freed and merged\n";
                }
            }
        } else {
            // Assume it's a block ID
            size_t blockId = std::stoull(arg);
            auto it = blockIdToAddress.find(blockId);
            if (it != blockIdToAddress.end()) {
                success = memoryManager->deallocate(it->second);
                if (success) {
                    addressToBlockId.erase(it->second);
                    blockIdToAddress.erase(blockId);
                    std::cout << "Block " << blockId << " freed and merged\n";
                }
            } else {
                std::cout << "Block ID " << blockId << " not found\n";
            }
        }
        
        if (!success && arg.substr(0, 2) != "0x" && arg.substr(0, 2) != "0X") {
            std::cout << "Failed to free block " << arg << "\n";
        }
    }
    
    void handleDump(const std::vector<std::string>& tokens) {
        if (!initialized) {
            std::cout << "Error: Memory not initialized.\n";
            return;
        }
        
        if (tokens.size() < 2 || tokens[1] != "memory") {
            std::cout << "Usage: dump memory\n";
            return;
        }
        
        memoryManager->dumpMemory();
    }
    
    void handleStats() {
        if (!initialized) {
            std::cout << "Error: Memory not initialized.\n";
            return;
        }
        
        // Update stats with current memory state
        statsManager->setFragmentationMetrics(
            memoryManager->getInternalFragmentation(),
            memoryManager->getExternalFragmentation(),
            memoryManager->getMemoryUtilization()
        );
        
        statsManager->setMemoryStats(
            memoryManager->getTotalMemory(),
            memoryManager->getUsedMemory(),
            memoryManager->getFreeMemory()
        );
        
        if (cacheSimulator) {
            statsManager->setCacheStats(
                cacheSimulator->getHits(1),
                cacheSimulator->getMisses(1),
                cacheSimulator->getHits(2),
                cacheSimulator->getMisses(2)
            );
        }
        
        statsManager->printStats();
        
        if (cacheSimulator) {
            cacheSimulator->printStatistics();
        }
    }
    
    void handleAccess(const std::vector<std::string>& tokens) {
        if (!initialized) {
            std::cout << "System not initialized. Use 'init memory <size>'\n";
            return;
        }
        
        if (tokens.size() < 2) {
            std::cout << "Usage: access <address>\n";
            return;
        }
        
        size_t physicalAddress = std::stoull(tokens[1], nullptr, 0);
        
        if (cacheSimulator) {
            // Access cache
            CacheSimulator::CacheAccessReport report = cacheSimulator->access(physicalAddress);
            
            std::cout << "Physical address 0x" << std::hex << physicalAddress << std::dec << "\n";
            std::cout << "  L1: " << (report.l1Hit ? "HIT" : "MISS") << "\n";
            if (!report.l1Hit) {
                std::cout << "  L2: " << (report.l2Accessed ? (report.l2Hit ? "HIT" : "MISS") : "-") << "\n";
            }
            
            // Print events (evictions)
            for (const auto& event : report.events) {
                std::cout << "  [!] " << event << "\n";
            }

            // Sync stats
            statsManager->setCacheStats(
                cacheSimulator->getHits(1),
                cacheSimulator->getMisses(1),
                cacheSimulator->getHits(2),
                cacheSimulator->getMisses(2)
            );
        } else {
             std::cout << "Cache simulator not initialized.\n";
        }
    }
};

int main() {
    MemorySimulatorCLI cli;
    cli.run();
    return 0;
}
