//
// Created by ofacklam on 17/12/2020.
//

#include "memorySegment.hpp"

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
