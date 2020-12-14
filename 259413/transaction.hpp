//
// Created by ofacklam on 14/12/2020.
//

#ifndef CS453_2020_PROJECT_TRANSACTION_HPP
#define CS453_2020_PROJECT_TRANSACTION_HPP

#include <cstdint>
#include <unordered_map>
#include <cstring>

#include "memoryRegion.hpp"
#include "block.hpp"

class Transaction {
private:
    std::unordered_map<void *, MemorySegment> allocated;
    std::unordered_map<void *, MemorySegment> freed;
    Blocks readCache;
    Blocks writeCache;
    bool isRO;
    bool isAborted;

public:
    explicit Transaction(bool isRo);

    bool read(Block toRead);

    bool write(Block toWrite);
};


#endif //CS453_2020_PROJECT_TRANSACTION_HPP
