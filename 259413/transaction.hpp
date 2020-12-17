//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_TRANSACTION_HPP
#define CS453_2020_PROJECT_TRANSACTION_HPP

#include <cstdint>
#include <unordered_map>
#include <cstring>
#include <queue>

#include "tm.hpp"
#include "memoryRegion.hpp"
#include "block.hpp"

class Commit {
public:
    Blocks written;
    std::unordered_map<void *, MemorySegment> freed;

public:
    explicit Commit(Blocks written);

    Commit(Blocks written, std::unordered_map<void *, MemorySegment> freed);
};

class Transaction {
private:
    std::unordered_map<void *, MemorySegment> allocated;
    std::unordered_map<void *, MemorySegment> freed;
    std::unordered_map<void *, MemorySegment> freedByOthers;
    Blocks readCache;
    Blocks writeCache;
    bool isRO;

    std::queue<Commit> outstandingCommits;
    std::mutex commitMutex;
    bool isAborted;

    bool handleOutstandingCommits();

    bool updateSnapshot(Commit c);

public:
    explicit Transaction(bool isRo);

    virtual ~Transaction();

    bool read(Block toRead);

    bool write(Block toWrite);

    Alloc allocate(size_t size, size_t alignment, void **target);

    bool free(MemoryRegion *memReg, void *segment);

    void handleNewCommit(const Blocks &written, std::unordered_map<void *, MemorySegment> freed);

    bool commit(MemoryRegion *memReg);

    void abort();
};


#endif //CS453_2020_PROJECT_TRANSACTION_HPP
