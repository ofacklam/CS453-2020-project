//
// Created by ofacklam on 14/12/2020.
//

#include "memoryRegion.hpp"

MemoryRegion::MemoryRegion(size_t firstSegmentSize, size_t alignment) : firstSegmentSize(firstSegmentSize),
                                                                        alignment(alignment) {
    // Allocate a first shared segment
    firstSegment = std::calloc(firstSegmentSize / alignment, alignment);
    if (firstSegment == nullptr)
        throw std::runtime_error("Could not allocate first segment");
}

MemoryRegion::~MemoryRegion() {
    // Free all remaining segments & first segment
    for (void *seg: segments) {
        free(seg);
    }
    free(firstSegment);
}
