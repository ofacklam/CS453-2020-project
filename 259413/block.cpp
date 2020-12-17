//
// Created by ofacklam on 14/12/2020.
//

#include "block.hpp"

Block::Block(uintptr_t begin, size_t size, void *data, bool isOwner)
        : isOwner(isOwner), begin(begin), size(size), data(data) {}

Block Block::copy() const {
    void *cp = malloc(size);
    std::memcpy(cp, reinterpret_cast<void *>(begin), size);
    return Block(begin, size, cp, true);
}

void Block::free() {
    if (isOwner)
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

void Blocks::add(Block block, bool copyData) {
    auto blockBegin = block.begin;
    auto blockEnd = blockBegin + block.size;

    // Find start of combined block
    // https://stackoverflow.com/a/41169879
    uintptr_t start;
    auto it = blocks.lower_bound(blockBegin);
    if (blocks.empty() || it == blocks.begin()) {
        start = blockBegin;
    } else {
        it--;
        auto prevEnd = it->second.begin + it->second.size;
        start = blockBegin <= prevEnd ? it->first : blockBegin;
    }

    // Find end of combined block
    uintptr_t end;
    uintptr_t extBegin;
    it = blocks.lower_bound(blockEnd);
    if (blocks.empty() || (it == blocks.begin() && it->first > blockEnd)) {
        end = blockEnd;
    } else {
        if (it == blocks.end() || it->first > blockEnd)
            it--;
        extBegin = it->second.begin;
        auto extEnd = extBegin + it->second.size;
        end = std::max(blockEnd, extEnd);
    }

    // Copy data if needed
    char *data = nullptr;
    if (copyData) {
        size_t size = end - start;
        data = static_cast<char *>(malloc(size));

        if (start < blockBegin)
            std::memcpy(data, blocks.at(start).data, blockBegin - start);
        if (blockEnd < end)
            std::memcpy(data + blockEnd - start,
                        static_cast<char *>(blocks.at(extBegin).data) + blockEnd - extBegin,
                        end - blockEnd);
        std::memcpy(data + blockBegin - start, block.data, block.size);
    }

    // Clean up all unused blocks
    block.free();
    it = blocks.lower_bound(start);
    while (it != blocks.end() && it->first < end) {
        it->second.free();
        it = blocks.erase(it);
    }

    // Create new block
    Block combined(start, end - start, data, copyData);
    blocks.emplace(start, combined);
}

Blocks Blocks::intersect(Block block) {
    auto blockBegin = block.begin;
    auto blockEnd = blockBegin + block.size;

    // Reference our blocks
    Blocks result;
    for (auto elem: blocks) {
        Block b = elem.second;
        auto begin = b.begin;
        auto end = begin + b.size;

        auto start = std::max(begin, blockBegin);
        auto finish = std::min(end, blockEnd);
        auto size = finish - start;
        if (size > 0) {
            auto data = static_cast<char *>(b.data);
            result.blocks.emplace(start, Block(start, size, data + start - begin));
        }
    }

    // Fill in gaps with shared blocks
    auto it = result.blocks.begin();
    auto begin = it == result.blocks.end() ? blockEnd : it->first;
    if (blockBegin < begin)
        result.blocks.emplace(blockBegin, Block(blockBegin, begin - blockBegin, reinterpret_cast<void *>(blockBegin)));

    while (it != result.blocks.end()) {
        auto end = it->first + it->second.size;
        auto next = std::next(it);
        auto nextBegin = next == result.blocks.end() ? blockEnd : next->first;
        if(nextBegin > end)
            result.blocks.emplace(end, Block(end, nextBegin - end, reinterpret_cast<void *>(end)));
        it++;
    }

    return result;
}

bool Blocks::overlaps(Block block) const {
    auto it = blocks.lower_bound(block.begin);
    bool disjointNext = it == blocks.end() || it->first >= block.begin + block.size;
    bool disjointPrev = it == blocks.end() || it == blocks.begin();
    if (!disjointPrev) {
        it--;
        disjointPrev |= it->first + it->second.size <= block.begin;
    }
    return !disjointNext || !disjointPrev;
}

bool Blocks::overlaps(const Blocks &otherBlocks) {
    return std::any_of(blocks.begin(), blocks.end(), [otherBlocks](std::pair<uintptr_t, Block> elem) {
        return otherBlocks.overlaps(elem.second);
    });
}

bool Blocks::overlapsAny(const std::unordered_map<void *, MemorySegment> &segments) const {
    return std::any_of(blocks.begin(), blocks.end(), [segments](std::pair<uintptr_t, Block> elem) {
        return elem.second.containedInAny(segments);
    });
}

Blocks Blocks::copy() const {
    Blocks result;
    for (auto elem: blocks) {
        Block cp = elem.second.copy();
        result.blocks.emplace(cp.begin, cp);
    }
    return result;
}

void Blocks::free() {
    for (auto b: blocks)
        b.second.free();
}
