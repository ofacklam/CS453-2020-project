//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_BLOCK_HPP
#define CS453_2020_PROJECT_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <algorithm>
#include <cstring>

#include "memoryRegion.hpp"

class Block {
private:
    bool isOwner;

public:
    uintptr_t begin;
    size_t size;
    void *data;

    /**
     * Constructors:
     * 1. reference to existing data, not owned
     * 2. newly allocated, owned
     * 3. copy of existing data, owned
     */
    Block(uintptr_t begin, size_t size, void *data, bool isOwner = false);

    Block copy() const;

    void free();

    bool containedIn(MemorySegment segment) const;

    bool containedInAny(const std::unordered_map<void *, MemorySegment> &segments) const;
};

class Blocks {
public:
    /**
     * Disjoint set of (ordered) blocks
     */
    std::map<uintptr_t, Block> blocks;

public:
    void add(Block block, bool copyData);

    Blocks intersect(Block block);

    bool overlaps(Blocks blocks);

    bool overlapsAny(const std::unordered_map<void *, MemorySegment> &segments) const;

    Blocks copy() const;

    void free();
};


#endif //CS453_2020_PROJECT_BLOCK_HPP
