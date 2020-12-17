//
// Created by ofacklam on 14/12/2020.
//

#include "transaction.hpp"

#include <utility>

Commit::Commit(Blocks written, std::unordered_map<void *, MemorySegment> freed)
        : written(std::move(written)), freed(std::move(freed)) {}

Transaction::Transaction(bool isRo) : isRO(isRo), isAborted(false) {}

bool Transaction::read(Block toRead) {
    if (isAborted || !handleOutstandingCommits())
        return false;

    // Check intersection with freed up segments
    if (!isRO && toRead.containedInAny(freedByOthers)) {
        abort();
        return false;
    }

    // Intersect block with write cache, then copy out data
    Blocks source = writeCache.intersect(toRead);
    char *dst = static_cast<char *>(toRead.data);
    for (auto elem: source.blocks) {
        Block blk = elem.second;
        void *src = blk.data;
        size_t size = blk.size;

        std::memcpy(dst, src, size);
        dst += size;
    }

    // If not read-only, add read block to read cache
    if (!isRO) {
        toRead.data = nullptr;
        readCache.add(toRead, false);
    }

    return true;
}

bool Transaction::write(Block toWrite) {
    if (isAborted || !handleOutstandingCommits())
        return false;

    // Check intersection with freed-up segments
    if (toWrite.containedInAny(freedByOthers)) {
        abort();
        return false;
    }

    // Check if write is in a newly allocated segment, or a shared segment
    for (auto segElem: allocated) {
        auto segment = segElem.second;

        // it is in this segment --> write directly
        if (toWrite.containedIn(segment)) {
            std::memcpy(reinterpret_cast<void *>(toWrite.begin), toWrite.data, toWrite.size);
            return true;
        }
    }

    // Not an allocated segment --> add write block to write cache
    writeCache.add(toWrite, true);
    return true;
}

Alloc Transaction::allocate(size_t size, size_t alignment, void **target) {
    if (isAborted || !handleOutstandingCommits())
        return Alloc::abort;

    // Try creating new segment
    try {
        MemorySegment segment(size, alignment);
        void *ptr = segment.data;
        allocated[ptr] = segment;
        *target = ptr;
        return Alloc::success;
    } catch (std::exception &e) {
        return Alloc::nomem;
    }
}

bool Transaction::free(MemoryRegion *memReg, void *segment) {
    if (isAborted || !handleOutstandingCommits())
        return false;

    // Check intersection with freed-up segments
    if (freedByOthers.count(segment) > 0) {
        abort();
        return false;
    }

    // Check if freeing allocated segment, or shared segment
    if (allocated.count(segment) > 0) {
        MemorySegment allocSeg = allocated[segment];
        allocSeg.free();
        allocated.erase(segment);
        return true;
    } else {
        freed[segment] = memReg->getMemorySegment(segment);
        return true;
    }
}

void Transaction::handleNewCommit(Blocks written, std::unordered_map<void *, MemorySegment> freedByOther) {
    std::unique_lock lock(commitMutex);

    if (isRO)
        written = written.copy();

    outstandingCommits.push(Commit(written, std::move(freedByOther)));
}

bool Transaction::handleOutstandingCommits() {
    std::unique_lock lock(commitMutex);

    while (!outstandingCommits.empty()) {
        Commit c = outstandingCommits.front();
        outstandingCommits.pop();
        if (!updateSnapshot(c)) {
            abort();
            return false;
        }
    }
    return true;
}

bool Transaction::updateSnapshot(Commit c) {
    return false;
}

void Transaction::commit(std::unordered_set<Transaction *> &otherTXs) {

}

void Transaction::abort() {
    isAborted = true;
}
