//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_TRANSACTION_HPP
#define CS453_2020_PROJECT_TRANSACTION_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <queue>
#include <cassert>
#include <mutex>

#include "tm.hpp"
#include "block.hpp"
#include "memorySegment.hpp"

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
    explicit Transaction(bool isRo, size_t alignment);

    virtual ~Transaction();

    bool read(Block toRead, const std::function<bool(std::function<bool()>)> &lockedForRead);

    bool write(Block toWrite);

    Alloc allocate(size_t size, size_t alignment, void **target);

    bool free(void *segment, std::function<MemorySegment(void *)> getMemorySegment);

    void handleNewCommit(const Blocks &written, std::unordered_map<void *, MemorySegment> freed,
                         const std::function<MemorySegment(void *)> &findMemorySegment);

    bool commit(const std::unordered_set<Transaction *> &txs,
                const std::function<void(MemorySegment)> &addMemorySegment,
                const std::function<void(void *)> &freeMemorySegment,
                const std::function<MemorySegment(void *)> &findMemorySegment);

    void abort();
};


#endif //CS453_2020_PROJECT_TRANSACTION_HPP
