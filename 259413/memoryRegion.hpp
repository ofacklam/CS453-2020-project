//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_MEMORYREGION_HPP
#define CS453_2020_PROJECT_MEMORYREGION_HPP

#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <stdexcept>

class Transaction;

class MemorySegment {
public:
    void *data;
    size_t size;

    MemorySegment(size_t size, size_t alignment);

    void free();
};

class MemoryRegion {
private:
    /**
     * Store memory segment pointers
     */
    std::unordered_map<void *, MemorySegment> segments;

public:
    MemorySegment firstSegment;
    size_t alignment;

    /**
     * Pending transactions
     */
    std::unordered_set<Transaction *> txs;

public:
    MemoryRegion(size_t firstSegmentSize, size_t alignment);

    virtual ~MemoryRegion();
};


#endif //CS453_2020_PROJECT_MEMORYREGION_HPP
