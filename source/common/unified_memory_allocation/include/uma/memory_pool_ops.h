/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef UMA_MEMORY_POOL_OPS_H
#define UMA_MEMORY_POOL_OPS_H 1

#include <uma/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/// \brief This structure comprises function pointers used by corresponding umaPool*
/// calls. Each memory pool implementation should initialize all function
/// pointers.
struct uma_memory_pool_ops_t {
    /// Version of the ops structure.
    /// Should be initialized using UMA_VERSION_CURRENT
    uint32_t version;

    ///
    /// \brief Intializes memory pool.
    /// \param providers array of memory providers that will be used for coarse-grain allocations.
    ///        Should contain at least one memory provider.
    /// \param numProvider number of elements in the providers array
    /// \param params pool-specific params
    /// \param pool [out] returns pointer to the pool
    /// \return UMA_RESULT_SUCCESS on success or appropriate error code on failure.
    enum uma_result_t (*initialize)(uma_memory_provider_handle_t *providers,
                                    size_t numProviders, void *params,
                                    void **pool);

    ///
    /// \brief Finalizes memory pool
    /// \param pool pool to finalize
    void (*finalize)(void *pool);

    /// Refer to memory_pool.h for description of those functions
    void *(*malloc)(void *pool, size_t size);
    void *(*calloc)(void *pool, size_t num, size_t size);
    void *(*realloc)(void *pool, void *ptr, size_t size);
    void *(*aligned_malloc)(void *pool, size_t size, size_t alignment);
    size_t (*malloc_usable_size)(void *pool, void *ptr);
    void (*free)(void *pool, void *);
    enum uma_result_t (*get_last_result)(void *pool, const char **ppMessage);
};

#ifdef __cplusplus
}
#endif

#endif /* UMA_MEMORY_POOL_OPS_H */
