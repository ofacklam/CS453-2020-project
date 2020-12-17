//
// Created by ofacklam on 17/12/2020.
//

#ifndef CS453_2020_PROJECT_MEMORYSEGMENT_HPP
#define CS453_2020_PROJECT_MEMORYSEGMENT_HPP

#include <cstdlib>
#include <stdexcept>

class MemorySegment {
public:
    void *data;
    size_t size;

    MemorySegment(size_t size, size_t alignment);

    void free();
};


#endif //CS453_2020_PROJECT_MEMORYSEGMENT_HPP
