//
// Created by ofacklam on 14/12/2020.
//

#include "block.hpp"

Block::Block(uintptr_t begin, size_t size, void *data, bool isOwner)
        : isOwner(isOwner), begin(begin), size(size), data(data) {}

Block Block::copy() const {
    void *cp = malloc(size);
    std::memcpy(cp, data, size);
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
    if (it == blocks.end() || it == blocks.begin()) {
        start = blockBegin;
    } else {
        it--;
        auto prevEnd = it->second.begin + it->second.size;
        start = blockBegin < prevEnd ? it->first : blockBegin;
    }

    // Find end of combined block
    uintptr_t end;
    uintptr_t extBegin;
    it = blocks.lower_bound(blockEnd);
    if (it == blocks.end() || (it->first > blockEnd && it == blocks.begin())) {
        end = blockEnd;
    } else {
        auto extBlock = it->first == blockEnd ? it : --it;
        extBegin = extBlock->second.begin;
        auto extEnd = extBegin + extBlock->second.size;
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
    while (it->first < end) {
        it->second.free();
        blocks.erase(it);
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
        if (begin >= blockBegin && begin < blockEnd) {
            auto size = std::min(b.size, blockEnd - begin);
            result.blocks.emplace(begin, Block(begin, size, b.data));
        }
    }

    // Fill in gaps with shared blocks
    uintptr_t current = blockBegin;
    while (current < blockEnd) {
        auto it = result.blocks.lower_bound(current);
        if (it == result.blocks.end()) {
            result.blocks.emplace(current, Block(current, blockEnd - current, reinterpret_cast<void *>(current)));
            break;
        }

        if (current < it->first) {
            result.blocks.emplace(current, Block(current, it->first - current, reinterpret_cast<void *>(current)));
        }
        current = it->first + it->second.size;
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
