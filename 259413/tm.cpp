/**
 * @file   tm.cpp
 * @author Oliver Facklam <oliver.facklam@epfl.ch>
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// Requested features
#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
#error Current C11 compiler does not support atomic operations
#endif

// External headers

// Internal headers
#include <tm.hpp>
#include "memoryRegion.hpp"
#include "transaction.hpp"

// -------------------------------------------------------------------------- //

/** Define a proposition as likely true.
 * @param prop Proposition
**/
#undef likely
#ifdef __GNUC__
#define likely(prop) \
        __builtin_expect((prop) ? 1 : 0, 1)
#else
#define likely(prop) \
        (prop)
#endif

/** Define a proposition as likely false.
 * @param prop Proposition
**/
#undef unlikely
#ifdef __GNUC__
#define unlikely(prop) \
        __builtin_expect((prop) ? 1 : 0, 0)
#else
#define unlikely(prop) \
        (prop)
#endif

/** Define one or several attributes.
 * @param type... Attribute names
**/
#undef as
#ifdef __GNUC__
#define as(type...) \
        __attribute__((type))
#else
#define as(type...)
#warning This compiler has no support for GCC attributes
#endif

// -------------------------------------------------------------------------- //

/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size, size_t align) noexcept {
    // Try to create a MemoryRegion object
    try {
        return new MemoryRegion(size, align);
    } catch (std::exception &e) {
        return invalid_shared;
    }
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared) noexcept {
    // Delete the MemoryRegion object (destructor will free memory segments)
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    delete memReg;
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void *tm_start(shared_t shared) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    return memReg->firstSegment.data;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    return memReg->firstSegment.size;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    return memReg->alignment;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared, bool is_ro) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *tx = new Transaction(is_ro, memReg->alignment);
    memReg->lockedForWrite([memReg, tx]() {
        memReg->txs.insert(tx);
        return true;
    });
    return (tx_t) tx;
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *transaction = reinterpret_cast<Transaction *>(tx);
    bool success = memReg->lockedForWrite([memReg, transaction]() {
        memReg->txs.erase(transaction);
        return transaction->commit(
                memReg->txs,
                [memReg](MemorySegment seg) { memReg->addMemorySegment(seg); },
                [memReg](void *ptr) { memReg->freeMemorySegment(ptr); },
                [memReg](void *ptr) { return memReg->findMemorySegment(ptr); }
        );
    });
    delete transaction;
    return success;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared, tx_t tx, void const *source, size_t size, void *target) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *transaction = reinterpret_cast<Transaction *>(tx);
    Block toRead(reinterpret_cast<uintptr_t>(source), size, target);
    bool success = transaction->read(toRead, [memReg](const std::function<bool()> &op) {
        return memReg->lockedForRead(op);
    });
    if (!success)
        memReg->deleteTransaction(transaction);
    return success;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared, tx_t tx, void const *source, size_t size, void *target) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *transaction = reinterpret_cast<Transaction *>(tx);
    // const cast https://stackoverflow.com/a/123995
    Block toWrite(reinterpret_cast<uintptr_t>(target), size, const_cast<void *>(source));
    bool success = transaction->write(toWrite);
    if (!success)
        memReg->deleteTransaction(transaction);
    return success;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
Alloc tm_alloc(shared_t shared, tx_t tx, size_t size, void **target) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *transaction = reinterpret_cast<Transaction *>(tx);
    auto success = transaction->allocate(size, memReg->alignment, target);
    if (success == Alloc::abort)
        memReg->deleteTransaction(transaction);
    return success;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t shared, tx_t tx, void *target) noexcept {
    auto *memReg = reinterpret_cast<MemoryRegion *>(shared);
    auto *transaction = reinterpret_cast<Transaction *>(tx);
    bool success = memReg->lockedForRead([memReg, transaction, target]() {
        return transaction->free(target, [memReg](void *ptr) {
            return memReg->getMemorySegment(ptr);
        });
    });
    if (!success)
        memReg->deleteTransaction(transaction);
    return success;
}
