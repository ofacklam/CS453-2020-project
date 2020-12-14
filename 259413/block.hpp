//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_BLOCK_HPP
#define CS453_2020_PROJECT_BLOCK_HPP

#include <cstddef>
#include <cstdint>
#include <map>

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
    Block(uintptr_t begin, size_t size, void *data);
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
    bool overlaps(Block block);
};


#endif //CS453_2020_PROJECT_BLOCK_HPP
