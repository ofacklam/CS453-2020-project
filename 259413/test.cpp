//
// Created by ofacklam on 17/12/2020.
//

#include <tm.hpp>
#include <iostream>

void print(int *arr, int n) {
    std::cout << "[";
    for(int i = 0; i < n; i++)
        std::cout << arr[i] << ", ";
    std::cout << "]" << std::endl;
}

int main() {
    auto n = 10;
    auto mem = tm_create(n * sizeof(int), sizeof(int));
    auto start = static_cast<int *>(tm_start(mem));

    auto init = tm_begin(mem, false);
    for (int i = 0; i < n; i++) {
        int val = 5;
        tm_write(mem, init, &val, sizeof(val), start + i);
    }
    print(start, n);
    tm_end(mem, init);
    print(start, n);

    auto reader = tm_begin(mem, true);
    auto writer1 = tm_begin(mem, false);
    for (int i = 0; i < n; i++) {
        int val = i;
        tm_write(mem, writer1, &val, sizeof(val), start + i);
    }
    print(start, n);
    tm_end(mem, writer1);
    print(start, n);

    int temp[n];
    for(int i = 0; i < n; i++) {
        int val;
        tm_read(mem, reader, start + i, sizeof(val), &val);
        temp[i] = val;
    }
    print(temp, n);
}
