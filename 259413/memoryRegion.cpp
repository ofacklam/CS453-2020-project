//
// Created by ofacklam on 14/12/2020.
//

#include "memoryRegion.hpp"

MemorySegment::MemorySegment(size_t size, size_t alignment) : size(size) {
    // Allocate the segment, respecting the given alignment
    data = std::calloc(size / alignment, alignment);
    if (data == nullptr)
        throw std::runtime_error("Could not allocate new segment");
}

void MemorySegment::free() {
    std::free(data);
    data = nullptr;
}

MemoryRegion::MemoryRegion(size_t firstSegmentSize, size_t alignment)
        : firstSegment(firstSegmentSize, alignment), alignment(alignment) {}

MemoryRegion::~MemoryRegion() {
    // Free all remaining segments & first segment
    for (auto segElem: segments) {
        segElem.second.free();
    }
    firstSegment.free();
}
