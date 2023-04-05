// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#ifndef UR_CONFORMANCE_INCLUDE_ENVIRONMENT_H_INCLUDED
#define UR_CONFORMANCE_INCLUDE_ENVIRONMENT_H_INCLUDED

#include <algorithm>
#include <gtest/gtest.h>
#include <string>
#include <unordered_map>
#include <ur_api.h>
namespace uur {

struct PlatformEnvironment : ::testing::Environment {

    struct PlatformOptions {
        std::string platform_name;
    };

    PlatformEnvironment(int argc, char **argv);
    virtual ~PlatformEnvironment() override = default;

    virtual void SetUp() override;
    virtual void TearDown() override;

    PlatformOptions parsePlatformOptions(int argc, char **argv);

    PlatformOptions platform_options;
    ur_platform_handle_t platform = nullptr;
    std::string error;
    static PlatformEnvironment *instance;
};

struct DevicesEnvironment : PlatformEnvironment {

    DevicesEnvironment(int argc, char **argv);
    virtual ~DevicesEnvironment() override = default;

    virtual void SetUp() override;
    virtual void TearDown() override;

    inline const std::vector<ur_device_handle_t> &GetDevices() const {
        return devices;
    }

    std::vector<ur_device_handle_t> devices;
    static DevicesEnvironment *instance;
};

struct KernelsEnvironment : DevicesEnvironment {
    struct KernelOptions {
        std::string kernel_directory;
    };

    KernelsEnvironment(int argc, char **argv, std::string kernels_default_dir);
    virtual ~KernelsEnvironment() override = default;

    virtual void SetUp() override;
    virtual void TearDown() override;

    ur_result_t LoadSource(const std::string &kernel_name,
                           uint32_t device_index,
                           std::shared_ptr<std::vector<char>> &binary_out);

    static KernelsEnvironment *instance;

  private:
    KernelOptions parseKernelOptions(int argc, char **argv,
                                     std::string kernels_default_dir);
    std::string getKernelSourcePath(const std::string &kernel_name,
                                    uint32_t device_index);
    std::string getSupportedILPostfix(uint32_t device_index);

    KernelOptions kernel_options;
    // mapping between kernels (full_path + kernel_name) and their saved source.
    std::unordered_map<std::string, std::shared_ptr<std::vector<char>>>
        cached_kernels;
};

} // namespace uur

#endif // UR_CONFORMANCE_INCLUDE_ENVIRONMENT_H_INCLUDED
