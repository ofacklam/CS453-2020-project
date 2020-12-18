//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_MEMORYREGION_HPP
#define CS453_2020_PROJECT_MEMORYREGION_HPP

#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <stdexcept>
#include <shared_mutex>
#include <functional>

#include "transaction.hpp"
#include "memorySegment.hpp"
#include "abstractMemoryRegion.hpp"

class MemoryRegion : public AbstractMemoryRegion {
private:
    /**
     * Store memory segment pointers
     */
    std::unordered_map<void *, MemorySegment> segments;

    /**
     * Mutex to lock for reading / writing shared memory (when reading or committing)
     */
    std::shared_mutex sharedMutex;

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

    bool lockedForRead(const std::function<bool()> &readOp) override;

    bool lockedForWrite(const std::function<bool()> &writeOp) override;

    MemorySegment getMemorySegment(void *ptr) override;

    MemorySegment findMemorySegment(void *ptr) override;

    void addMemorySegment(MemorySegment segment) override;

    void freeMemorySegment(void *ptr) override;

    void deleteTransaction(Transaction *tx) override;
};


#endif //CS453_2020_PROJECT_MEMORYREGION_HPP
