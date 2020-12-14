//
// Created by ofacklam on 14/12/2020.
//

#include "transaction.hpp"

Transaction::Transaction(bool isRo) : isRO(isRo), isAborted(false) {}

bool Transaction::read(Block toRead) {
    if (isAborted)
        return false;

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
    if(isAborted)
        return false;

    // Check if write is in a newly allocated segment, or a shared segment
    for (auto segElem: allocated) {
        auto segment = segElem.second;
        auto segBegin = reinterpret_cast<uintptr_t>(segment.data);
        uintptr_t segEnd = segBegin + segment.size;

        // it is in this segment --> write directly
        if (toWrite.begin >= segBegin && toWrite.begin < segEnd) {
            std::memcpy(reinterpret_cast<void *>(toWrite.begin), toWrite.data, toWrite.size);
            return true;
        }
    }

    // Not an allocated segment --> add write block to write cache
    writeCache.add(toWrite, true);
    return true;
}
