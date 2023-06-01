// Copyright (C) 2023 Intel Corporation
// Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
// See LICENSE.TXT
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "fixtures.hpp"

TEST_F(adapterRegSearchTest, testSearchOrder) {
    // Adapter search order:
    // 1. Every path from UR_ADAPTERS_SEARCH_PATH.
    // 2. OS search paths.
    // 3. Loader library directory.

    auto it = std::find_if(registry.cbegin(), registry.cend(), hasTestLibName);
    ASSERT_NE(it, registry.end());

    auto testAdapterPaths = *it;
    auto pathIt = std::find_if(testAdapterPaths.cbegin(),
                               testAdapterPaths.cend(), isTestEnvPath);
    std::size_t index = std::distance(testAdapterPaths.cbegin(), pathIt);
    ASSERT_EQ(index, 0);

    pathIt = std::find_if(testAdapterPaths.cbegin(), testAdapterPaths.cend(),
                          isTestLibName);
    index = std::distance(testAdapterPaths.cbegin(), pathIt);
    ASSERT_EQ(index, 1);

    pathIt = std::find_if(testAdapterPaths.cbegin(), testAdapterPaths.cend(),
                          isCurPath);
    index = std::distance(testAdapterPaths.cbegin(), pathIt);
    ASSERT_EQ(index, 2);
}
