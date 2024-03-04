/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
 * See LICENSE.TXT
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * @file ur_sanitizer_utils.hpp
 *
 */

#pragma once

#include "common.hpp"

namespace ur_sanitizer_layer {

ur_context_handle_t GetContext(ur_queue_handle_t Queue);
ur_device_handle_t GetDevice(ur_queue_handle_t Queue);
ur_program_handle_t GetProgram(ur_kernel_handle_t Kernel);
size_t GetLocalMemorySize(ur_device_handle_t Device);
std::string GetKernelName(ur_kernel_handle_t Kernel);
ur_device_handle_t GetUSMAllocDevice(ur_context_handle_t Context,
                                     const void *MemPtr);
DeviceType GetDeviceType(ur_device_handle_t Device);
std::vector<ur_device_handle_t> GetProgramDevices(ur_program_handle_t Program);

} // namespace ur_sanitizer_layer
