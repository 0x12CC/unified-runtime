// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include <uur/fixtures.h>

struct urDeviceReleaseTest : uur::urAllDevicesTest {};

TEST_F(urDeviceReleaseTest, Success) {
    for (auto device : devices) {
        ASSERT_SUCCESS(urDeviceRetain(device));
        EXPECT_SUCCESS(urDeviceRelease(device));
    }
}

TEST_F(urDeviceReleaseTest, InvalidNullHandle) {
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                     urDeviceRelease(nullptr));
}
