//
// Created by ofacklam on 14/12/2020.
//

#include "transaction.hpp"

#include <utility>

Commit::Commit(Blocks written, std::unordered_map<void *, MemorySegment> freed)
        : written(std::move(written)), freed(std::move(freed)) {}

Commit::Commit(Blocks written) : written(std::move(written)) {}

Transaction::Transaction(bool isRo, size_t alignment)
        : readCache(alignment), writeCache(alignment), isRO(isRo), isAborted(false) {}

Transaction::~Transaction() {
    readCache.free();
    writeCache.free();
}

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
        allocated.emplace(ptr, segment);
        *target = ptr;
        return Alloc::success;
    } catch (std::exception &e) {
        return Alloc::nomem;
    }
}

bool Transaction::free(void *segment, std::function<MemorySegment(void *)> getMemorySegment) {
    if (isAborted || !handleOutstandingCommits())
        return false;

    // Check intersection with freed-up segments
    if (freedByOthers.count(segment) > 0) {
        abort();
        return false;
    }

    // Check if freeing allocated segment, or shared segment
    if (allocated.count(segment) > 0) {
        MemorySegment allocSeg = allocated.at(segment);
        allocSeg.free();
        allocated.erase(segment);
        return true;
    } else {
        freed.emplace(segment, getMemorySegment(segment));
        return true;
    }
}

void Transaction::handleNewCommit(const Blocks &written, std::unordered_map<void *, MemorySegment> freedByOther) {
    std::unique_lock lock(commitMutex);

    if (isRO) {
        Blocks modified = written.copy();
        for (auto elem: freedByOther) {
            auto segment = elem.second;
            auto begin = reinterpret_cast<uintptr_t>(segment.data);
            modified.add(Block(begin, segment.size, segment.data), true);
        }
        outstandingCommits.push(Commit(modified));
    } else {
        outstandingCommits.push(Commit(written, std::move(freedByOther)));
    }
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
    if (isRO) {
        Blocks prevCache = writeCache;
        writeCache = c.written;
        for (auto b: prevCache.blocks)
            writeCache.add(b.second, true);
        return true;
    } else {
        if (c.written.overlaps(readCache))
            return false;
        if (writeCache.overlapsAny(c.freed) || readCache.overlapsAny(c.freed))
            return false;

        for (auto s: c.freed) {
            if (freed.count(s.first) > 0)
                return false;
            freedByOthers.emplace(s.first, s.second);
        }
        return true;
    }
}

bool
Transaction::commit(const std::unordered_set<Transaction *> &txs, std::function<void(MemorySegment)> addMemorySegment,
                    std::function<void(void *)> freeMemorySegment) {
    if (isAborted || !handleOutstandingCommits())
        return false;

    if (isRO)
        return true;

    // Let all other transactions handle this new commit
    for (auto tx: txs) {
        if (tx != this)
            tx->handleNewCommit(writeCache, freed);
    }

    // Add allocated segments
    for (auto newSegment: allocated) {
        addMemorySegment(newSegment.second);
    }

    // Write contents of writeCache
    for (auto elem: writeCache.blocks) {
        Block b = elem.second;
        std::memcpy(reinterpret_cast<void *>(b.begin), b.data, b.size);
    }

    // Free "freed" segments
    for (auto freedSegment: freed) {
        freeMemorySegment(freedSegment.first);
    }

    return true;
}

void Transaction::abort() {
    // Free allocated segments
    for (auto s: allocated)
        s.second.free();

    // Free outstanding commits
    while (!outstandingCommits.empty()) {
        auto c = outstandingCommits.front();
        outstandingCommits.pop();
        if (isRO)
            c.written.free();
    }

    isAborted = true;
}
