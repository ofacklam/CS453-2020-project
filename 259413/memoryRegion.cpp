//
// Created by ofacklam on 14/12/2020.
//

#include "memoryRegion.hpp"

MemoryRegion::MemoryRegion(size_t firstSegmentSize, size_t alignment)
        : firstSegment(firstSegmentSize, alignment), alignment(alignment) {}

MemoryRegion::~MemoryRegion() {
    // Free all remaining segments & first segment
    for (auto segElem: segments) {
        segElem.second.free();
    }
    firstSegment.free();
}

bool MemoryRegion::lockedForRead(const std::function<bool()>& readOp) {
    std::shared_lock lock(sharedMutex);
    return readOp();
}

bool MemoryRegion::lockedForWrite(const std::function<bool()> &writeOp) {
    std::unique_lock lock(sharedMutex);
    return writeOp();
}

MemorySegment MemoryRegion::getMemorySegment(void *ptr) {
    return segments.at(ptr);
}

void MemoryRegion::addMemorySegment(MemorySegment segment) {
    segments.emplace(segment.data, segment);
}

void MemoryRegion::freeMemorySegment(void *ptr) {
    segments.at(ptr).free();
    segments.erase(ptr);
}

void MemoryRegion::deleteTransaction(Transaction *tx) {
    lockedForWrite([tx, this]() {
        delete tx;
        this->txs.erase(tx);
        return true;
    });
}
