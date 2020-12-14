//
// Created by ofacklam on 14/12/2020.
//

#include "block.hpp"

Block::Block(uintptr_t begin, size_t size, void *data) : isOwner(false), begin(begin), size(size), data(data) {}
