// Copyright (C) 2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include <uur/fixtures.h>

using urQueueGetInfoTest = uur::urQueueTestWithParam<ur_queue_info_t>;

UUR_TEST_SUITE_P(urQueueGetInfoTest,
                 ::testing::Values(UR_QUEUE_INFO_CONTEXT, UR_QUEUE_INFO_DEVICE, UR_QUEUE_INFO_DEVICE_DEFAULT, UR_QUEUE_INFO_PROPERTIES,
                                   UR_QUEUE_INFO_REFERENCE_COUNT, UR_QUEUE_INFO_SIZE),
                 uur::deviceTestWithParamPrinter<ur_queue_info_t>);

TEST_P(urQueueGetInfoTest, Success) {
    ur_queue_info_t info_type = getParam();
    size_t size = 0;
    ASSERT_SUCCESS(urQueueGetInfo(queue, info_type, 0, nullptr, &size));
    ASSERT_NE(size, 0);
    std::vector<uint8_t> data(size);
    ASSERT_SUCCESS(urQueueGetInfo(queue, info_type, size, data.data(), nullptr));
}
