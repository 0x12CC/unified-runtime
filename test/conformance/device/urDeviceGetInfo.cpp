// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT

#include <uur/fixtures.h>

struct urDeviceGetInfoTest : uur::urAllDevicesTest,
                             ::testing::WithParamInterface<ur_device_info_t> {

    void SetUp() override {
        UUR_RETURN_ON_FATAL_FAILURE(uur::urAllDevicesTest::SetUp());
    }
};

INSTANTIATE_TEST_SUITE_P(
    , urDeviceGetInfoTest,
    ::testing::Values(

        UR_DEVICE_INFO_TYPE,
        UR_DEVICE_INFO_VENDOR_ID,                              //
        UR_DEVICE_INFO_DEVICE_ID,                              //
        UR_DEVICE_INFO_MAX_COMPUTE_UNITS,                      //
        UR_DEVICE_INFO_MAX_WORK_ITEM_DIMENSIONS,               //
        UR_DEVICE_INFO_MAX_WORK_ITEM_SIZES,                    //
        UR_DEVICE_INFO_MAX_WORK_GROUP_SIZE,                    //
        UR_DEVICE_INFO_SINGLE_FP_CONFIG,                       //
        UR_DEVICE_INFO_HALF_FP_CONFIG,                         //
        UR_DEVICE_INFO_DOUBLE_FP_CONFIG,                       //
        UR_DEVICE_INFO_QUEUE_PROPERTIES,                       //
        UR_DEVICE_INFO_PREFERRED_VECTOR_WIDTH_CHAR,            //
        UR_DEVICE_INFO_PREFERRED_VECTOR_WIDTH_SHORT,           //
        UR_DEVICE_INFO_PREFERRED_VECTOR_WIDTH_LONG,            //
        UR_DEVICE_INFO_PREFERRED_VECTOR_WIDTH_FLOAT,           //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_CHAR,               //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_SHORT,              //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_INT,                //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_LONG,               //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_FLOAT,              //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_DOUBLE,             //
        UR_DEVICE_INFO_NATIVE_VECTOR_WIDTH_HALF,               //
        UR_DEVICE_INFO_MAX_CLOCK_FREQUENCY,                    //
        UR_DEVICE_INFO_MEMORY_CLOCK_RATE,                      //
        UR_DEVICE_INFO_ADDRESS_BITS,                           //
        UR_DEVICE_INFO_MAX_MEM_ALLOC_SIZE,                     //
        UR_DEVICE_INFO_IMAGE_SUPPORTED,                        //
        UR_DEVICE_INFO_MAX_READ_IMAGE_ARGS,                    //
        UR_DEVICE_INFO_MAX_WRITE_IMAGE_ARGS,                   //
        UR_DEVICE_INFO_MAX_READ_WRITE_IMAGE_ARGS,              //
        UR_DEVICE_INFO_IMAGE2D_MAX_WIDTH,                      //
        UR_DEVICE_INFO_IMAGE2D_MAX_HEIGHT,                     //
        UR_DEVICE_INFO_IMAGE3D_MAX_WIDTH,                      //
        UR_DEVICE_INFO_IMAGE3D_MAX_HEIGHT,                     //
        UR_DEVICE_INFO_IMAGE3D_MAX_DEPTH,                      //
        UR_DEVICE_INFO_IMAGE_MAX_BUFFER_SIZE,                  //
        UR_DEVICE_INFO_IMAGE_MAX_ARRAY_SIZE,                   //
        UR_DEVICE_INFO_MAX_SAMPLERS,                           //
        UR_DEVICE_INFO_MAX_PARAMETER_SIZE,                     //
        UR_DEVICE_INFO_MEM_BASE_ADDR_ALIGN,                    //
        UR_DEVICE_INFO_GLOBAL_MEM_CACHE_TYPE,                  //
        UR_DEVICE_INFO_GLOBAL_MEM_CACHELINE_SIZE,              //
        UR_DEVICE_INFO_GLOBAL_MEM_CACHE_SIZE,                  //
        UR_DEVICE_INFO_GLOBAL_MEM_SIZE,                        //
        UR_DEVICE_INFO_GLOBAL_MEM_FREE,                        //
        UR_DEVICE_INFO_MAX_CONSTANT_BUFFER_SIZE,               //
        UR_DEVICE_INFO_MAX_CONSTANT_ARGS,                      //
        UR_DEVICE_INFO_LOCAL_MEM_TYPE,                         //
        UR_DEVICE_INFO_LOCAL_MEM_SIZE,                         //
        UR_DEVICE_INFO_ERROR_CORRECTION_SUPPORT,               //
        UR_DEVICE_INFO_HOST_UNIFIED_MEMORY,                    //
        UR_DEVICE_INFO_PROFILING_TIMER_RESOLUTION,             //
        UR_DEVICE_INFO_ENDIAN_LITTLE,                          //
        UR_DEVICE_INFO_AVAILABLE,                              //
        UR_DEVICE_INFO_COMPILER_AVAILABLE,                     //
        UR_DEVICE_INFO_LINKER_AVAILABLE,                       //
        UR_DEVICE_INFO_EXECUTION_CAPABILITIES,                 //
        UR_DEVICE_INFO_QUEUE_ON_DEVICE_PROPERTIES,             //
        UR_DEVICE_INFO_QUEUE_ON_HOST_PROPERTIES,               //
        UR_DEVICE_INFO_BUILT_IN_KERNELS,                       //
        UR_DEVICE_INFO_PLATFORM,                               //
        UR_DEVICE_INFO_REFERENCE_COUNT,                        //
        UR_DEVICE_INFO_IL_VERSION,                             //
        UR_DEVICE_INFO_NAME,                                   //
        UR_DEVICE_INFO_VENDOR,                                 //
        UR_DEVICE_INFO_DRIVER_VERSION,                         //
        UR_DEVICE_INFO_PROFILE,                                //
        UR_DEVICE_INFO_VERSION,                                //
        UR_DEVICE_INFO_BACKEND_RUNTIME_VERSION,                //
        UR_DEVICE_INFO_EXTENSIONS,                             //
        UR_DEVICE_INFO_PRINTF_BUFFER_SIZE,                     //
        UR_DEVICE_INFO_PREFERRED_INTEROP_USER_SYNC,            //
        UR_DEVICE_INFO_PARENT_DEVICE,                          //
        UR_DEVICE_INFO_PARTITION_PROPERTIES,                   //
        UR_DEVICE_INFO_PARTITION_MAX_SUB_DEVICES,              //
        UR_DEVICE_INFO_PARTITION_AFFINITY_DOMAIN,              //
        UR_DEVICE_INFO_PARTITION_TYPE,                         //
        UR_DEVICE_INFO_MAX_NUM_SUB_GROUPS,                     //
        UR_DEVICE_INFO_SUB_GROUP_INDEPENDENT_FORWARD_PROGRESS, //
        UR_DEVICE_INFO_SUB_GROUP_SIZES_INTEL,                  //
        UR_DEVICE_INFO_USM_HOST_SUPPORT,                       //
        UR_DEVICE_INFO_USM_DEVICE_SUPPORT,                     //
        UR_DEVICE_INFO_USM_SINGLE_SHARED_SUPPORT,              //
        UR_DEVICE_INFO_USM_CROSS_SHARED_SUPPORT,               //
        UR_DEVICE_INFO_USM_SYSTEM_SHARED_SUPPORT,              //
        UR_DEVICE_INFO_UUID,                                   //
        UR_DEVICE_INFO_PCI_ADDRESS,                            //
        UR_DEVICE_INFO_GPU_EU_COUNT,                           //
        UR_DEVICE_INFO_GPU_EU_SIMD_WIDTH,                      //
        UR_DEVICE_INFO_GPU_EU_SLICES,                          //
        UR_DEVICE_INFO_GPU_SUBSLICES_PER_SLICE,                //
        UR_DEVICE_INFO_MAX_MEMORY_BANDWIDTH,                   //
        UR_DEVICE_INFO_IMAGE_SRGB,                             //
        UR_DEVICE_INFO_ATOMIC_64,                              //
        UR_DEVICE_INFO_ATOMIC_MEMORY_ORDER_CAPABILITIES,       //
        UR_DEVICE_INFO_BFLOAT16,                               //
        UR_DEVICE_INFO_MAX_COMPUTE_QUEUE_INDICES               //
        ),
    [](const ::testing::TestParamInfo<ur_device_info_t> &info) {
        std::stringstream ss;
        ss << info.param;
        return ss.str();
    });

TEST_P(urDeviceGetInfoTest, Success) {
    ur_device_info_t info_type = GetParam();
    for (auto device : devices) {
        size_t size = 0;
        ASSERT_SUCCESS(urDeviceGetInfo(device, info_type, 0, nullptr, &size));
        ASSERT_NE(size, 0);
        void *info_data = alloca(size);
        ASSERT_SUCCESS(
            urDeviceGetInfo(device, info_type, size, info_data, nullptr));
        ASSERT_NE(info_data, nullptr);
    }
}

TEST_P(urDeviceGetInfoTest, InvalidNullHandleDevice) {
    ur_device_type_t device_type;
    ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_NULL_HANDLE,
                     urDeviceGetInfo(nullptr, UR_DEVICE_INFO_TYPE,
                                     sizeof(ur_device_type_t), &device_type,
                                     nullptr));
}

TEST_P(urDeviceGetInfoTest, InvalidEnumerationInfoType) {
    for (auto device : devices) {
        ur_device_type_t device_type;
        ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_ENUMERATION,
                         urDeviceGetInfo(device, UR_DEVICE_INFO_FORCE_UINT32,
                                         sizeof(ur_device_type_t), &device_type,
                                         nullptr));
    }
}

TEST_P(urDeviceGetInfoTest, InvalidValuePropSize) {
    for (auto device : devices) {
        ur_device_type_t device_type;
        ASSERT_EQ_RESULT(UR_RESULT_ERROR_INVALID_VALUE,
                         urDeviceGetInfo(device, UR_DEVICE_INFO_TYPE, 0,
                                         &device_type, nullptr));
    }
}
