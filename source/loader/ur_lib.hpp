/*
 *
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
 * See LICENSE.TXT
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * @file ur_lib.hpp
 *
 */

#ifndef UR_LOADER_LIB_H
#define UR_LOADER_LIB_H 1

#include "ur_api.h"
#include "ur_ddi.h"
#include "ur_util.hpp"
#include <mutex>
#include <vector>

namespace ur_lib {
///////////////////////////////////////////////////////////////////////////////
class context_t {
  public:
#ifdef DYNAMIC_LOAD_LOADER
    HMODULE loader = nullptr;
#endif

    context_t();
    ~context_t();

    std::once_flag initOnce;

    ur_result_t Init(ur_device_init_flags_t dflags);

    ur_result_t urInit();
    ur_dditable_t urDdiTable = {};
};

extern context_t *context;

} // namespace ur_lib

#endif /* UR_LOADER_LIB_H */
