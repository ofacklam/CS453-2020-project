//
// Created by ofacklam on 14/12/2020.
//

#include "block.hpp"

Block::Block(uintptr_t begin, size_t size, void *data, bool isOwner) : isOwner(isOwner), begin(begin), size(size), data(data) {}

Block Block::copy() const {
    void *cp = malloc(size);
    std::memcpy(cp, data, size);
    return Block(begin, size, cp, true);
}

void Block::free() {
    if(isOwner)
        std::free(data);
}

bool Block::containedIn(MemorySegment segment) const {
    auto segBegin = reinterpret_cast<uintptr_t>(segment.data);
    uintptr_t segEnd = segBegin + segment.size;
    return begin >= segBegin && begin < segEnd;
}

bool Block::containedInAny(const std::unordered_map<void *, MemorySegment> &segments) const {
    return std::any_of(segments.begin(), segments.end(), [this](std::pair<void *const, MemorySegment> elem) {
        return containedIn(elem.second);
    });
}

Blocks Blocks::copy() const {
    Blocks result;
    for(auto elem: blocks) {
        Block cp = elem.second.copy();
        result.blocks[cp.begin] = cp;
    }
    return result;
}

void Blocks::free() {
    for(auto b: blocks)
        b.second.free();
}
