//
// Created by ofacklam on 14/12/2020.
//

#include "block.hpp"

Block::Block(uintptr_t begin, size_t size, void *data, bool isOwner)
        : isOwner(isOwner), begin(begin), size(size), data(data) {}

/*Block::Block() {
    throw std::runtime_error("No block default-allocation!");
}*/

Block Block::copy(size_t alignment) const {
    void *cp = aligned_alloc(alignment, size);
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

bool Block::ownsData() const {
    return isOwner;
}

Blocks::Blocks(size_t alignment) : alignment(alignment) {}

void Blocks::add(Block block, bool copyData) {
    auto blockBegin = block.begin;
    auto blockEnd = blockBegin + block.size;

    // Find combined block ends
    uintptr_t start = blockBegin;
    uintptr_t extBegin = 0;
    uintptr_t end = blockEnd;
    for (auto elem: blocks) {
        Block b = elem.second;
        auto bEnd = b.begin + b.size;
        if (b.begin < start && bEnd >= blockBegin)
            start = b.begin;
        if (bEnd > end && b.begin <= blockEnd) {
            end = bEnd;
            extBegin = b.begin;
        }
    }

    // Copy data if needed
    size_t size = end - start;
    char *data = static_cast<char *>(block.data);
    if (copyData && (!block.ownsData() || size > block.size)) {
        data = static_cast<char *>(aligned_alloc(alignment, size));

        if (start < blockBegin)
            std::memcpy(data, blocks.at(start).data, blockBegin - start);
        if (blockEnd < end)
            std::memcpy(data + blockEnd - start,
                        static_cast<char *>(blocks.at(extBegin).data) + blockEnd - extBegin,
                        end - blockEnd);
        std::memcpy(data + blockBegin - start, block.data, block.size);

        block.free();
    }

    // Clean up all unused blocks
    for (auto it = blocks.begin(); it != blocks.end();) {
        Block b = it->second;
        if (b.begin >= start && b.begin + b.size <= end) {
            b.free();
            it = blocks.erase(it);
        } else {
            it++;
        }
    }

    // Create new block
    Block combined(start, size, data, copyData);
    blocks.emplace(start, combined);
}

Blocks Blocks::intersect(Block block) {
    auto blockBegin = block.begin;
    auto blockEnd = blockBegin + block.size;

    // Reference our blocks
    Blocks result(alignment);
    for (auto elem: blocks) {
        Block b = elem.second;
        auto begin = b.begin;
        auto end = begin + b.size;

        auto start = std::max(begin, blockBegin);
        auto finish = std::min(end, blockEnd);
        if (finish > start) {
            auto data = static_cast<char *>(b.data);
            result.blocks.emplace(start, Block(start, finish - start, data + start - begin));
        }
    }

    // Fill in gaps with shared blocks
    /*auto it = result.blocks.begin();
    auto begin = it == result.blocks.end() ? blockEnd : it->first;
    if (blockBegin < begin)
        result.blocks.emplace(blockBegin, Block(blockBegin, begin - blockBegin, reinterpret_cast<void *>(blockBegin)));

    while (it != result.blocks.end()) {
        auto end = it->first + it->second.size;
        auto next = std::next(it);
        auto nextBegin = next == result.blocks.end() ? blockEnd : next->first;
        if (nextBegin > end)
            result.blocks.emplace(end, Block(end, nextBegin - end, reinterpret_cast<void *>(end)));
        it++;
    }*/

    return result;
}

bool Blocks::overlaps(Block block) const {
    /*auto it = blocks.lower_bound(block.begin);
    bool disjointNext = it == blocks.end() || it->first >= block.begin + block.size;
    bool disjointPrev = it == blocks.end() || it == blocks.begin();
    if (!disjointPrev) {
        it--;
        disjointPrev |= it->first + it->second.size <= block.begin;
    }
    return !disjointNext || !disjointPrev;*/
    for (auto elem: blocks) {
        Block b = elem.second;
        if (b.begin < block.begin + block.size && block.begin < b.begin + b.size)
            return true;
    }
    return false;
}

bool Blocks::overlaps(const Blocks &otherBlocks) {
    for (auto elem: blocks) {
        if (otherBlocks.overlaps(elem.second))
            return true;
    }
    return false;
}

bool Blocks::overlapsAny(const std::unordered_map<void *, MemorySegment> &segments) const {
    for (auto elem: blocks) {
        if (elem.second.containedInAny(segments))
            return true;
    }
    return false;
}

const Block *Blocks::contains(Block block) const {
    for (auto &elem: blocks) {
        const Block &b = elem.second;
        if (b.begin <= block.begin && b.begin + b.size >= block.begin + block.size)
            return &b;
    }
    return nullptr;

    /*
     *     if(blocks.empty())
        return nullptr;

    auto it = blocks.lower_bound(block.begin);
    if (it == blocks.end() || it->second.begin > block.begin)
        it--;

    const Block &b = it->second;
    if(b.begin <= block.begin && b.begin + b.size >= block.begin + block.size)
        return &b;
    else
        return nullptr;
     */
}

Blocks Blocks::copy() const {
    Blocks result(alignment);
    for (auto elem: blocks) {
        Block cp = elem.second.copy(alignment);
        result.blocks.emplace(cp.begin, cp);
    }
    return result;
}

void Blocks::free() {
    for (auto b: blocks)
        b.second.free();
}
