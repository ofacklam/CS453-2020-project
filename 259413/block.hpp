//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_BLOCK_HPP
#define CS453_2020_PROJECT_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstring>

#include "memorySegment.hpp"

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
     * 2. copy of existing data, owned
     */
    Block(uintptr_t begin, size_t size, void *data, bool isOwner = false);

    // Default constructor
    Block() = default;

    Block copy(size_t alignment) const;

    void free();

    bool containedIn(MemorySegment segment) const;

    bool containedInAny(const std::unordered_map<void *, MemorySegment> &segments) const;

    bool ownsData() const;
};

class Blocks {
public:
    size_t alignment;

public:
    /**
     * Disjoint set of (ordered) blocks
     */
    std::map<uintptr_t, Block> blocks;
    bool firstValid = false;
    Block first;

public:
    explicit Blocks(size_t alignment);

    void add(Block block, bool copyData);

    Blocks intersect(Block block);

    bool overlaps(Block block) const;

    bool overlaps(const Blocks &otherBlocks);

    bool overlapsAny(const std::unordered_map<void *, MemorySegment> &segments) const;

    const Block *contains(Block block) const;

    Blocks copy() const;

    void free();
};


#endif //CS453_2020_PROJECT_BLOCK_HPP
