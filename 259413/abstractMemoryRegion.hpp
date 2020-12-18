//
// Created by ofacklam on 18/12/2020.
//

#ifndef CS453_2020_PROJECT_ABSTRACTMEMORYREGION_HPP
#define CS453_2020_PROJECT_ABSTRACTMEMORYREGION_HPP

#include <functional>

class Transaction;

class AbstractMemoryRegion {
public:
    virtual bool lockedForRead(const std::function<bool()> &readOp) = 0;

    virtual bool lockedForWrite(const std::function<bool()> &writeOp) = 0;

    virtual MemorySegment getMemorySegment(void *ptr) = 0;

    virtual MemorySegment findMemorySegment(void *ptr) = 0;

    virtual void addMemorySegment(MemorySegment segment) = 0;

    virtual void freeMemorySegment(void *ptr) = 0;

    virtual void deleteTransaction(Transaction *tx) = 0;
};


#endif //CS453_2020_PROJECT_ABSTRACTMEMORYREGION_HPP
