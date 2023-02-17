// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include <uur/fixtures.h>

using urMemGetInfoTest = uur::urMemBufferTestWithParam<ur_mem_info_t>;

UUR_TEST_SUITE_P(urMemGetInfoTest, ::testing::Values(UR_MEM_INFO_SIZE, UR_MEM_INFO_CONTEXT),
                 uur::deviceTestWithParamPrinter<ur_mem_info_t>);

TEST_P(urMemGetInfoTest, Success) {
    ur_mem_info_t info = getParam();
    size_t size;
    ASSERT_SUCCESS(urMemGetInfo(buffer, info, 0, nullptr, &size));
    ASSERT_NE(size, 0);
    std::vector<uint8_t> info_data(size);
    ASSERT_SUCCESS(urMemGetInfo(buffer, info, size, info_data.data(), nullptr));
}

TEST_P(urMemGetInfoTest, InvalidNullHandleMemory) {
    size_t mem_size = 0;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                     urMemGetInfo(nullptr, UR_MEM_INFO_SIZE, sizeof(size_t),
                                  &mem_size, nullptr));
}

TEST_P(urMemGetInfoTest, InvalidEnumerationMemInfoType) {
    size_t mem_size = 0;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                     urMemGetInfo(buffer, UR_MEM_INFO_FORCE_UINT32,
                                  sizeof(size_t), &mem_size, nullptr));
}
