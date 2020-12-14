//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_MEMORYREGION_HPP
#define CS453_2020_PROJECT_MEMORYREGION_HPP

#include <unordered_set>
#include <cstdlib>
#include <stdexcept>


class MemoryRegion {
private:
    /**
     * Store memory segment pointers
     */
    std::unordered_set<void *> segments;

public:
    void *firstSegment;
    size_t firstSegmentSize;
    size_t alignment;

public:
    MemoryRegion(size_t firstSegmentSize, size_t alignment);

    virtual ~MemoryRegion();
};


#endif //CS453_2020_PROJECT_MEMORYREGION_HPP
