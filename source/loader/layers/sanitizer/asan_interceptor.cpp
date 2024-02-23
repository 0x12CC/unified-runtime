//===----------------------------------------------------------------------===//
/*
 *
 * Copyright (C) 2023 Intel Corporation
 *
 * Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
 * See LICENSE.TXT
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * @file asan_interceptor.cpp
 *
 */

#include "asan_interceptor.hpp"
#include "asan_shadow_setup.hpp"
#include "device_sanitizer_report.hpp"
#include "stacktrace.hpp"

namespace ur_sanitizer_layer {

namespace {

// These magic values are written to shadow for better error
// reporting.
constexpr int kUsmDeviceRedzoneMagic = (char)0x81;
constexpr int kUsmHostRedzoneMagic = (char)0x82;
constexpr int kUsmSharedRedzoneMagic = (char)0x83;
constexpr int kMemBufferRedzoneMagic = (char)0x84;

const int kUsmDeviceDeallocatedMagic = (char)0x91;
const int kUsmHostDeallocatedMagic = (char)0x92;
const int kUsmSharedDeallocatedMagic = (char)0x93;
const int kMemBufferDeallocatedMagic = (char)0x93;

constexpr auto kSPIR_AsanShadowMemoryGlobalStart =
    "__AsanShadowMemoryGlobalStart";
constexpr auto kSPIR_AsanShadowMemoryGlobalEnd = "__AsanShadowMemoryGlobalEnd";
constexpr auto kSPIR_AsanShadowMemoryLocalStart =
    "__AsanShadowMemoryLocalStart";
constexpr auto kSPIR_AsanShadowMemoryLocalEnd = "__AsanShadowMemoryLocalEnd";

constexpr auto kSPIR_DeviceType = "__DeviceType";

constexpr auto kSPIR_DeviceSanitizerReportMem = "__DeviceSanitizerReportMem";

DeviceSanitizerReport SPIR_DeviceSanitizerReportMem;

uptr MemToShadow_CPU(uptr USM_SHADOW_BASE, uptr UPtr) {
    return USM_SHADOW_BASE + (UPtr >> 3);
}

uptr MemToShadow_PVC(uptr USM_SHADOW_BASE, uptr UPtr) {
    if (UPtr & 0xFF00000000000000ULL) { // Device USM
        return USM_SHADOW_BASE + 0x200000000000ULL +
               ((UPtr & 0xFFFFFFFFFFFFULL) >> 3);
    } else { // Only consider 47bit VA
        return USM_SHADOW_BASE + ((UPtr & 0x7FFFFFFFFFFFULL) >> 3);
    }
}

ur_context_handle_t getContext(ur_queue_handle_t Queue) {
    ur_context_handle_t Context{};
    [[maybe_unused]] auto Result = context.urDdiTable.Queue.pfnGetInfo(
        Queue, UR_QUEUE_INFO_CONTEXT, sizeof(ur_context_handle_t), &Context,
        nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getContext() failed");
    return Context;
}

ur_device_handle_t getDevice(ur_queue_handle_t Queue) {
    ur_device_handle_t Device{};
    [[maybe_unused]] auto Result = context.urDdiTable.Queue.pfnGetInfo(
        Queue, UR_QUEUE_INFO_DEVICE, sizeof(ur_device_handle_t), &Device,
        nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getDevice() failed");
    return Device;
}

ur_program_handle_t getProgram(ur_kernel_handle_t Kernel) {
    ur_program_handle_t Program{};
    [[maybe_unused]] auto Result = context.urDdiTable.Kernel.pfnGetInfo(
        Kernel, UR_KERNEL_INFO_PROGRAM, sizeof(ur_program_handle_t), &Program,
        nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getProgram() failed");
    return Program;
}

size_t getLocalMemorySize(ur_device_handle_t Device) {
    size_t LocalMemorySize{};
    [[maybe_unused]] auto Result = context.urDdiTable.Device.pfnGetInfo(
        Device, UR_DEVICE_INFO_LOCAL_MEM_SIZE, sizeof(LocalMemorySize),
        &LocalMemorySize, nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getLocalMemorySize() failed");
    return LocalMemorySize;
}

std::string getKernelName(ur_kernel_handle_t Kernel) {
    size_t KernelNameSize = 0;
    [[maybe_unused]] auto Result = context.urDdiTable.Kernel.pfnGetInfo(
        Kernel, UR_KERNEL_INFO_FUNCTION_NAME, 0, nullptr, &KernelNameSize);
    assert(Result == UR_RESULT_SUCCESS && "getKernelName() failed");

    std::vector<char> KernelNameBuf(KernelNameSize);
    Result = context.urDdiTable.Kernel.pfnGetInfo(
        Kernel, UR_KERNEL_INFO_FUNCTION_NAME, KernelNameSize,
        KernelNameBuf.data(), nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getKernelName() failed");

    return std::string(KernelNameBuf.data(), KernelNameSize - 1);
}

ur_device_handle_t getUSMAllocDevice(ur_context_handle_t Context,
                                     const void *MemPtr) {
    ur_device_handle_t Device{};
    // if urGetMemAllocInfo failed, return nullptr
    context.urDdiTable.USM.pfnGetMemAllocInfo(Context, MemPtr,
                                              UR_USM_ALLOC_INFO_DEVICE,
                                              sizeof(Device), &Device, nullptr);
    return Device;
}

DeviceType getDeviceType(ur_device_handle_t Device) {
    ur_device_type_t DeviceType = UR_DEVICE_TYPE_DEFAULT;
    [[maybe_unused]] auto Result = context.urDdiTable.Device.pfnGetInfo(
        Device, UR_DEVICE_INFO_TYPE, sizeof(DeviceType), &DeviceType, nullptr);
    assert(Result == UR_RESULT_SUCCESS && "getDeviceType() failed");
    switch (DeviceType) {
    case UR_DEVICE_TYPE_CPU:
        return DeviceType::CPU;
    case UR_DEVICE_TYPE_GPU: {
        // TODO: Check device name
        return DeviceType::GPU_PVC;
    }
    default:
        return DeviceType::UNKNOWN;
    }
}

const char *getFormatString(MemoryType MemoryType) {
    switch (MemoryType) {
    case MemoryType::DEVICE_USM:
        return "USM Device Memory";
    case MemoryType::HOST_USM:
        return "USM Host Memory";
    case MemoryType::SHARED_USM:
        return "USM Shared Memory";
    case MemoryType::MEM_BUFFER:
        return "Memory Buffer";
    default:
        return "Unknown Memory";
    }
}

void ReportBadFree(uptr Addr, StackTrace stack,
                   std::shared_ptr<USMAllocInfo> AllocInfo) {
    context.logger.always(
        "\n====ERROR: DeviceSanitizer: attempting free on address which "
        "was not malloc()-ed: {} in thread T0",
        (void *)Addr);
    stack.Print();

    if (!AllocInfo) { // maybe Addr is host allocated memory
        context.logger.always("{} is maybe allocated on Host Memory",
                              (void *)Addr);
        exit(1);
    }

    assert(!AllocInfo->IsReleased && "Chunk must be not released");

    context.logger.always("{} is located inside of {} region [{}, {}]",
                          (void *)Addr, getFormatString(AllocInfo->Type),
                          (void *)AllocInfo->UserBegin,
                          (void *)AllocInfo->UserEnd);
    context.logger.always("allocated by thread T0 here:");
    AllocInfo->AllocStack.Print();

    exit(1);
}

void ReportDoubleFree(uptr Addr, StackTrace Stack,
                      std::shared_ptr<USMAllocInfo> AllocInfo) {
    context.logger.always("\n====ERROR: DeviceSanitizer: double-free on {}",
                          (void *)Addr);
    Stack.Print();
    AllocInfo->AllocStack.Print();
    AllocInfo->ReleaseStack.Print();
    exit(1);
}

void ReportGenericError(DeviceSanitizerReport &Report,
                        ur_kernel_handle_t Kernel, ur_context_handle_t Context,
                        ur_device_handle_t Device) {
    const char *File = Report.File[0] ? Report.File : "<unknown file>";
    const char *Func = Report.Func[0] ? Report.Func : "<unknown func>";
    auto KernelName = getKernelName(Kernel);

    context.logger.always("\n====ERROR: DeviceSanitizer: {} on {}",
                          DeviceSanitizerFormat(Report.ErrorType),
                          DeviceSanitizerFormat(Report.MemoryType));
    context.logger.always(
        "{} of size {} at kernel <{}> LID({}, {}, {}) GID({}, "
        "{}, {})",
        Report.IsWrite ? "WRITE" : "READ", Report.AccessSize,
        KernelName.c_str(), Report.LID0, Report.LID1, Report.LID2, Report.GID0,
        Report.GID1, Report.GID2);
    context.logger.always("  #0 {} {}:{}\n", Func, File, Report.Line);

    if (Report.ErrorType == DeviceSanitizerErrorType::USE_AFTER_FREE) {
        auto AllocInfos = context.interceptor->findAllocInfoByAddress(
            Report.Addr, Context, Device);
        if (!AllocInfos.size()) {
            context.logger.always("can't find which chunck {} is allocated",
                                  (void *)Report.Addr);
        }
        for (auto &AllocInfo : AllocInfos) {
            if (!AllocInfo->IsReleased) {
                continue;
            }
            context.logger.always(
                "{} is located inside of {} region [{}, {}]",
                (void *)Report.Addr, getFormatString(AllocInfo->Type),
                (void *)AllocInfo->UserBegin, (void *)AllocInfo->UserEnd);
            context.logger.always("allocated by thread T0 here:");
            AllocInfo->AllocStack.Print();
            context.logger.always("released by thread T0 here:");
            AllocInfo->ReleaseStack.Print();
        }
    }

    exit(1);
}

} // namespace

SanitizerInterceptor::SanitizerInterceptor()
    : m_IsInASanContext(IsInASanContext()),
      m_ShadowMemInited(m_IsInASanContext) {}

SanitizerInterceptor::~SanitizerInterceptor() {
    DestroyShadowMemoryOnCPU();
    DestroyShadowMemoryOnPVC();
}

/// The memory chunk allocated from the underlying allocator looks like this:
/// L L L L L L U U U U U U R R
///   L -- left redzone words (0 or more bytes)
///   U -- user memory.
///   R -- right redzone (0 or more bytes)
///
/// ref: "compiler-rt/lib/asan/asan_allocator.cpp" Allocator::Allocate
ur_result_t SanitizerInterceptor::allocateMemory(
    ur_context_handle_t Context, ur_device_handle_t Device,
    const ur_usm_desc_t *Properties, ur_usm_pool_handle_t Pool, size_t Size,
    void **ResultPtr, MemoryType Type) {
    auto Alignment = Properties->align;
    assert(Alignment == 0 || IsPowerOfTwo(Alignment));

    auto ContextInfo = getContextInfo(Context);
    std::shared_ptr<DeviceInfo> DeviceInfo =
        Device ? getDeviceInfo(Device) : nullptr;

    if (Alignment == 0) {
        Alignment =
            DeviceInfo ? DeviceInfo->Alignment : ASAN_SHADOW_GRANULARITY;
    }

    // Copy from LLVM compiler-rt/lib/asan
    uptr RZLog = ComputeRZLog(Size);
    uptr RZSize = RZLog2Size(RZLog);
    uptr RoundedSize = RoundUpTo(Size, Alignment);
    uptr NeededSize = RoundedSize + RZSize * 2;

    void *Allocated = nullptr;

    if (Type == MemoryType::DEVICE_USM) {
        UR_CALL(context.urDdiTable.USM.pfnDeviceAlloc(
            Context, Device, Properties, Pool, NeededSize, &Allocated));
    } else if (Type == MemoryType::HOST_USM) {
        UR_CALL(context.urDdiTable.USM.pfnHostAlloc(Context, Properties, Pool,
                                                    NeededSize, &Allocated));
    } else if (Type == MemoryType::SHARED_USM) {
        UR_CALL(context.urDdiTable.USM.pfnSharedAlloc(
            Context, Device, Properties, Pool, NeededSize, &Allocated));
    } else {
        context.logger.error("Unsupport memory type");
        return UR_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Copy from LLVM compiler-rt/lib/asan
    uptr AllocBegin = reinterpret_cast<uptr>(Allocated);
    [[maybe_unused]] uptr AllocEnd = AllocBegin + NeededSize;
    uptr UserBegin = AllocBegin + RZSize;
    if (!IsAligned(UserBegin, Alignment)) {
        UserBegin = RoundUpTo(UserBegin, Alignment);
    }
    uptr UserEnd = UserBegin + Size;
    assert(UserEnd <= AllocEnd);

    *ResultPtr = reinterpret_cast<void *>(UserBegin);

    auto AllocInfo =
        std::make_shared<USMAllocInfo>(USMAllocInfo{AllocBegin,
                                                    UserBegin,
                                                    UserEnd,
                                                    NeededSize,
                                                    Type,
                                                    false,
                                                    Context,
                                                    Device,
                                                    GetCurrentBacktrace(),
                                                    {}});

    // For updating shadow memory
    if (Device) { // Device/Shared USM
        ContextInfo->insertAllocInfo({Device}, AllocInfo);
    } else { // Host USM
        ContextInfo->insertAllocInfo(ContextInfo->DeviceList, AllocInfo);
    }

    // For memory release
    {
        std::scoped_lock<ur_shared_mutex> Guard(m_AllocationsMapMutex);
        m_AllocationsMap.emplace(std::move(AllocInfo));
    }

    context.logger.info(
        "AllocInfos(AllocBegin={},  User={}-{}, NeededSize={}, Type={})",
        (void *)AllocBegin, (void *)UserBegin, (void *)UserEnd, NeededSize,
        Type);

    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::releaseMemory(ur_context_handle_t Context,
                                                void *Ptr) {
    auto ContextInfo = getContextInfo(Context);

    auto Addr = reinterpret_cast<uptr>(Ptr);
    auto AllocInfos = findAllocInfoByAddress(Addr, Context, nullptr);

    if (!AllocInfos.size()) {
        ReportBadFree(Addr, GetCurrentBacktrace(), nullptr);
        return UR_RESULT_ERROR_INVALID_ARGUMENT;
    }

    for (auto AllocInfo : AllocInfos) {
        context.logger.debug("AllocInfo(AllocBegin={}, UserBegin={})",
                             (void *)AllocInfo->AllocBegin,
                             (void *)AllocInfo->UserBegin);

        if (AllocInfo->IsReleased) {
            ReportDoubleFree(Addr, GetCurrentBacktrace(), AllocInfo);
            return UR_RESULT_ERROR_INVALID_ARGUMENT;
        }

        if (Addr != AllocInfo->UserBegin) {
            ReportBadFree(Addr, GetCurrentBacktrace(), AllocInfo);
            return UR_RESULT_ERROR_INVALID_ARGUMENT;
        }

        AllocInfo->IsReleased = true;
        AllocInfo->ReleaseStack = GetCurrentBacktrace();

        auto Device =
            getUSMAllocDevice(Context, (const void *)AllocInfo->AllocBegin);

        // TODO: Check Device
        // TODO: Quarantine Cache

        // auto Res =
        //     context.urDdiTable.USM.pfnFree(Context, (void *)AllocInfo->AllocBegin);
        // if (Res != UR_RESULT_SUCCESS) {
        //     return Res;
        // }

        if (AllocInfo->Type == MemoryType::HOST_USM) {
            ContextInfo->insertAllocInfo(ContextInfo->DeviceList, AllocInfo);
        } else {
            ContextInfo->insertAllocInfo({Device}, AllocInfo);
        }
    }

    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::preLaunchKernel(ur_kernel_handle_t Kernel,
                                                  ur_queue_handle_t Queue,
                                                  LaunchInfo &LaunchInfo,
                                                  uint32_t numWorkgroup) {
    auto Context = getContext(Queue);
    auto Device = getDevice(Queue);
    auto ContextInfo = getContextInfo(Context);
    auto DeviceInfo = getDeviceInfo(Device);

    ManagedQueue InternalQueue(Context, Device);
    if (!InternalQueue) {
        return UR_RESULT_ERROR_INVALID_QUEUE;
    }

    UR_CALL(prepareLaunch(Context, DeviceInfo, InternalQueue, Kernel,
                          LaunchInfo, numWorkgroup));

    UR_CALL(updateShadowMemory(ContextInfo, DeviceInfo, InternalQueue));

    UR_CALL(context.urDdiTable.Queue.pfnFinish(InternalQueue));

    return UR_RESULT_SUCCESS;
}

void SanitizerInterceptor::postLaunchKernel(ur_kernel_handle_t Kernel,
                                            ur_queue_handle_t Queue,
                                            ur_event_handle_t &Event,
                                            LaunchInfo &LaunchInfo) {
    auto Program = getProgram(Kernel);
    ur_event_handle_t ReadEvent{};

    // If kernel has defined SPIR_DeviceSanitizerReportMem, then we try to read it
    // to host, but it's okay that it isn't defined
    // FIXME: We must use block operation here, until we support urEventSetCallback
    auto Result = context.urDdiTable.Enqueue.pfnDeviceGlobalVariableRead(
        Queue, Program, kSPIR_DeviceSanitizerReportMem, true,
        sizeof(LaunchInfo.SPIR_DeviceSanitizerReportMem), 0,
        &LaunchInfo.SPIR_DeviceSanitizerReportMem, 1, &Event, &ReadEvent);

    if (Result == UR_RESULT_SUCCESS) {
        Event = ReadEvent;

        auto AH = &LaunchInfo.SPIR_DeviceSanitizerReportMem;
        if (!AH->Flag) {
            return;
        }
        ReportGenericError(*AH, Kernel, getContext(Queue), getDevice(Queue));
    }
}

ur_result_t DeviceInfo::allocShadowMemory(ur_context_handle_t Context) {
    if (Type == DeviceType::CPU) {
        UR_CALL(SetupShadowMemoryOnCPU(ShadowOffset, ShadowOffsetEnd));
    } else if (Type == DeviceType::GPU_PVC) {
        UR_CALL(SetupShadowMemoryOnPVC(Context, ShadowOffset, ShadowOffsetEnd));
    } else {
        context.logger.error("Unsupport device type");
        return UR_RESULT_ERROR_INVALID_ARGUMENT;
    }
    context.logger.info("ShadowMemory(Global): {} - {}", (void *)ShadowOffset,
                        (void *)ShadowOffsetEnd);
    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::enqueueMemSetShadow(
    ur_context_handle_t Context, std::shared_ptr<DeviceInfo> &DeviceInfo,
    ur_queue_handle_t Queue, uptr Ptr, uptr Size, u8 Value) {

    if (DeviceInfo->Type == DeviceType::CPU) {
        uptr ShadowBegin = MemToShadow_CPU(DeviceInfo->ShadowOffset, Ptr);
        uptr ShadowEnd =
            MemToShadow_CPU(DeviceInfo->ShadowOffset, Ptr + Size - 1);

        // Poison shadow memory outside of asan runtime is not allowed, so we
        // need to avoid memset's call from being intercepted.
        static auto MemSet =
            (void *(*)(void *, int, size_t))GetMemFunctionPointer("memset");
        if (!MemSet) {
            return UR_RESULT_ERROR_UNKNOWN;
        }

        MemSet((void *)ShadowBegin, Value, ShadowEnd - ShadowBegin + 1);
        context.logger.debug(
            "enqueueMemSetShadow (addr={}, count={}, value={})",
            (void *)ShadowBegin, ShadowEnd - ShadowBegin + 1,
            (void *)(size_t)Value);
    } else if (DeviceInfo->Type == DeviceType::GPU_PVC) {
        uptr ShadowBegin = MemToShadow_PVC(DeviceInfo->ShadowOffset, Ptr);
        uptr ShadowEnd =
            MemToShadow_PVC(DeviceInfo->ShadowOffset, Ptr + Size - 1);

        {
            static const size_t PageSize = [Context, &DeviceInfo]() {
                size_t Size;
                [[maybe_unused]] auto Result =
                    context.urDdiTable.VirtualMem.pfnGranularityGetInfo(
                        Context, DeviceInfo->Handle,
                        UR_VIRTUAL_MEM_GRANULARITY_INFO_RECOMMENDED,
                        sizeof(Size), &Size, nullptr);
                assert(Result == UR_RESULT_SUCCESS);
                context.logger.info("PVC PageSize: {}", Size);
                return Size;
            }();

            ur_physical_mem_properties_t Desc{
                UR_STRUCTURE_TYPE_PHYSICAL_MEM_PROPERTIES, nullptr, 0};
            static ur_physical_mem_handle_t PhysicalMem{};

            // Make sure [Ptr, Ptr + Size] is mapped to physical memory
            for (auto MappedPtr = RoundDownTo(ShadowBegin, PageSize);
                 MappedPtr <= ShadowEnd; MappedPtr += PageSize) {
                if (!PhysicalMem) {
                    auto URes = context.urDdiTable.PhysicalMem.pfnCreate(
                        Context, DeviceInfo->Handle, PageSize, &Desc,
                        &PhysicalMem);
                    if (URes != UR_RESULT_SUCCESS) {
                        context.logger.error("urPhysicalMemCreate(): {}", URes);
                        return URes;
                    }
                }

                context.logger.debug("urVirtualMemMap: {} ~ {}",
                                     (void *)MappedPtr,
                                     (void *)(MappedPtr + PageSize - 1));

                // FIXME: No flag to check the failed reason is VA is already mapped
                auto URes = context.urDdiTable.VirtualMem.pfnMap(
                    Context, (void *)MappedPtr, PageSize, PhysicalMem, 0,
                    UR_VIRTUAL_MEM_ACCESS_FLAG_READ_WRITE);
                if (URes != UR_RESULT_SUCCESS) {
                    context.logger.debug("urVirtualMemMap(): {}", URes);
                }

                // Initialize to zero
                if (URes == UR_RESULT_SUCCESS) {
                    // Reset PhysicalMem to null since it's been mapped
                    PhysicalMem = nullptr;

                    const char Pattern[] = {0};

                    auto URes = context.urDdiTable.Enqueue.pfnUSMFill(
                        Queue, (void *)MappedPtr, 1, Pattern, PageSize, 0,
                        nullptr, nullptr);
                    if (URes != UR_RESULT_SUCCESS) {
                        context.logger.error("urEnqueueUSMFill(): {}", URes);
                        return URes;
                    }
                }
            }
        }

        const char Pattern[] = {(char)Value};
        auto URes = context.urDdiTable.Enqueue.pfnUSMFill(
            Queue, (void *)ShadowBegin, 1, Pattern, ShadowEnd - ShadowBegin + 1,
            0, nullptr, nullptr);
        context.logger.debug(
            "enqueueMemSetShadow (addr={}, count={}, value={}): {}",
            (void *)ShadowBegin, ShadowEnd - ShadowBegin + 1,
            (void *)(size_t)Value, URes);
        if (URes != UR_RESULT_SUCCESS) {
            context.logger.error("urEnqueueUSMFill(): {}", URes);
            return URes;
        }
    } else {
        context.logger.error("Unsupport device type");
        return UR_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return UR_RESULT_SUCCESS;
}

/// Each 8 bytes of application memory are mapped into one byte of shadow memory
/// The meaning of that byte:
///  - Negative: All bytes are not accessible (poisoned)
///  - 0: All bytes are accessible
///  - 1 <= k <= 7: Only the first k bytes is accessible
///
/// ref: https://github.com/google/sanitizers/wiki/AddressSanitizerAlgorithm#mapping
ur_result_t SanitizerInterceptor::enqueueAllocInfo(
    ur_context_handle_t Context, std::shared_ptr<DeviceInfo> &DeviceInfo,
    ur_queue_handle_t Queue, std::shared_ptr<USMAllocInfo> &AllocInfo) {
    if (AllocInfo->IsReleased) {
        int ShadowByte;
        switch (AllocInfo->Type) {
        case MemoryType::HOST_USM:
            ShadowByte = kUsmHostDeallocatedMagic;
            break;
        case MemoryType::DEVICE_USM:
            ShadowByte = kUsmDeviceDeallocatedMagic;
            break;
        case MemoryType::SHARED_USM:
            ShadowByte = kUsmSharedDeallocatedMagic;
            break;
        case MemoryType::MEM_BUFFER:
            ShadowByte = kMemBufferDeallocatedMagic;
            break;
        default:
            ShadowByte = 0xff;
            assert(false && "Unknow AllocInfo Type");
        }
        UR_CALL(enqueueMemSetShadow(Context, DeviceInfo, Queue,
                                    AllocInfo->AllocBegin, AllocInfo->AllocSize,
                                    ShadowByte));
        return UR_RESULT_SUCCESS;
    }

    // Init zero
    UR_CALL(enqueueMemSetShadow(Context, DeviceInfo, Queue,
                                AllocInfo->AllocBegin, AllocInfo->AllocSize,
                                0));

    uptr TailBegin = RoundUpTo(AllocInfo->UserEnd, ASAN_SHADOW_GRANULARITY);
    uptr TailEnd = AllocInfo->AllocBegin + AllocInfo->AllocSize;

    // User tail
    if (TailBegin != AllocInfo->UserEnd) {
        auto Value = AllocInfo->UserEnd -
                     RoundDownTo(AllocInfo->UserEnd, ASAN_SHADOW_GRANULARITY);
        UR_CALL(enqueueMemSetShadow(Context, DeviceInfo, Queue,
                                    AllocInfo->UserEnd, 1,
                                    static_cast<u8>(Value)));
    }

    int ShadowByte;
    switch (AllocInfo->Type) {
    case MemoryType::HOST_USM:
        ShadowByte = kUsmHostRedzoneMagic;
        break;
    case MemoryType::DEVICE_USM:
        ShadowByte = kUsmDeviceRedzoneMagic;
        break;
    case MemoryType::SHARED_USM:
        ShadowByte = kUsmSharedRedzoneMagic;
        break;
    case MemoryType::MEM_BUFFER:
        ShadowByte = kMemBufferRedzoneMagic;
        break;
    default:
        ShadowByte = 0xff;
        assert(false && "Unknow AllocInfo Type");
    }

    // Left red zone
    UR_CALL(enqueueMemSetShadow(
        Context, DeviceInfo, Queue, AllocInfo->AllocBegin,
        AllocInfo->UserBegin - AllocInfo->AllocBegin, ShadowByte));

    // Right red zone
    UR_CALL(enqueueMemSetShadow(Context, DeviceInfo, Queue, TailBegin,
                                TailEnd - TailBegin, ShadowByte));

    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::updateShadowMemory(
    std::shared_ptr<ContextInfo> &ContextInfo,
    std::shared_ptr<DeviceInfo> &DeviceInfo, ur_queue_handle_t Queue) {
    auto &AllocInfos = ContextInfo->AllocInfosMap[DeviceInfo->Handle];
    std::scoped_lock<ur_shared_mutex> Guard(AllocInfos.Mutex);

    for (auto &AllocInfo : AllocInfos.List) {
        UR_CALL(enqueueAllocInfo(ContextInfo->Handle, DeviceInfo, Queue,
                                 AllocInfo));
    }
    AllocInfos.List.clear();

    return UR_RESULT_SUCCESS;
}

ur_result_t
SanitizerInterceptor::insertContext(ur_context_handle_t Context,
                                    std::shared_ptr<ContextInfo> &CI) {
    std::scoped_lock<ur_shared_mutex> Guard(m_ContextMapMutex);

    if (m_ContextMap.find(Context) != m_ContextMap.end()) {
        CI = m_ContextMap.at(Context);
        return UR_RESULT_SUCCESS;
    }

    CI = std::make_shared<ContextInfo>(Context);

    m_ContextMap.emplace(Context, CI);

    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::eraseContext(ur_context_handle_t Context) {
    std::scoped_lock<ur_shared_mutex> Guard(m_ContextMapMutex);
    assert(m_ContextMap.find(Context) != m_ContextMap.end());
    m_ContextMap.erase(Context);
    // TODO: Remove devices in each context
    return UR_RESULT_SUCCESS;
}

ur_result_t
SanitizerInterceptor::insertDevice(ur_device_handle_t Device,
                                   std::shared_ptr<DeviceInfo> &DI) {
    std::scoped_lock<ur_shared_mutex> Guard(m_DeviceMapMutex);

    if (m_DeviceMap.find(Device) != m_DeviceMap.end()) {
        DI = m_DeviceMap.at(Device);
        return UR_RESULT_SUCCESS;
    }

    DI = std::make_shared<ur_sanitizer_layer::DeviceInfo>(Device);

    // Query device type
    DI->Type = getDeviceType(Device);
    if (DI->Type == DeviceType::UNKNOWN) {
        return UR_RESULT_ERROR_UNSUPPORTED_FEATURE;
    }

    // Query alignment
    UR_CALL(context.urDdiTable.Device.pfnGetInfo(
        Device, UR_DEVICE_INFO_MEM_BASE_ADDR_ALIGN, sizeof(DI->Alignment),
        &DI->Alignment, nullptr));

    m_DeviceMap.emplace(Device, DI);

    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::eraseDevice(ur_device_handle_t Device) {
    std::scoped_lock<ur_shared_mutex> Guard(m_DeviceMapMutex);
    assert(m_DeviceMap.find(Device) != m_DeviceMap.end());
    m_DeviceMap.erase(Device);
    // TODO: Remove devices in each context
    return UR_RESULT_SUCCESS;
}

ur_result_t SanitizerInterceptor::prepareLaunch(
    ur_context_handle_t Context, std::shared_ptr<DeviceInfo> &DeviceInfo,
    ur_queue_handle_t Queue, ur_kernel_handle_t Kernel, LaunchInfo &LaunchInfo,
    uint32_t numWorkgroup) {
    auto Program = getProgram(Kernel);

    do {
        // Set global variable to program
        auto EnqueueWriteGlobal = [&Queue, &Program](const char *Name,
                                                     const void *Value) {
            auto Result =
                context.urDdiTable.Enqueue.pfnDeviceGlobalVariableWrite(
                    Queue, Program, Name, false, sizeof(uptr), 0, Value, 0,
                    nullptr, nullptr);
            if (Result != UR_RESULT_SUCCESS) {
                context.logger.warning("Device Global[{}] Write Failed: {}",
                                       Name, Result);
                return false;
            }
            return true;
        };

        // Write shadow memory offset for global memory
        EnqueueWriteGlobal(kSPIR_AsanShadowMemoryGlobalStart,
                           &DeviceInfo->ShadowOffset);
        EnqueueWriteGlobal(kSPIR_AsanShadowMemoryGlobalEnd,
                           &DeviceInfo->ShadowOffsetEnd);

        // Write device type
        EnqueueWriteGlobal(kSPIR_DeviceType, &DeviceInfo->Type);

        if (DeviceInfo->Type == DeviceType::CPU) {
            break;
        }

        // Write shadow memory offset for local memory
        auto LocalMemorySize = getLocalMemorySize(DeviceInfo->Handle);
        auto LocalShadowMemorySize =
            (numWorkgroup * LocalMemorySize) >> ASAN_SHADOW_SCALE;

        context.logger.info("LocalInfo(WorkGroup={}, LocalMemorySize={}, "
                            "LocalShadowMemorySize={})",
                            numWorkgroup, LocalMemorySize,
                            LocalShadowMemorySize);

        ur_usm_desc_t Desc{UR_STRUCTURE_TYPE_USM_HOST_DESC, nullptr, 0, 0};
        auto Result = context.urDdiTable.USM.pfnDeviceAlloc(
            Context, DeviceInfo->Handle, &Desc, nullptr, LocalShadowMemorySize,
            (void **)&LaunchInfo.LocalShadowOffset);
        if (Result != UR_RESULT_SUCCESS) {
            context.logger.error(
                "Failed to allocate shadow memory for local memory: {}",
                numWorkgroup, Result);
            context.logger.error("Maybe the number of workgroup too large");
            return Result;
        }
        LaunchInfo.LocalShadowOffsetEnd =
            LaunchInfo.LocalShadowOffset + LocalShadowMemorySize - 1;

        EnqueueWriteGlobal(kSPIR_AsanShadowMemoryLocalStart,
                           &LaunchInfo.LocalShadowOffset);
        EnqueueWriteGlobal(kSPIR_AsanShadowMemoryLocalEnd,
                           &LaunchInfo.LocalShadowOffsetEnd);

        {
            const char Pattern[] = {0};

            auto URes = context.urDdiTable.Enqueue.pfnUSMFill(
                Queue, (void *)LaunchInfo.LocalShadowOffset, 1, Pattern,
                LocalShadowMemorySize, 0, nullptr, nullptr);
            if (URes != UR_RESULT_SUCCESS) {
                context.logger.error("urEnqueueUSMFill(): {}", URes);
                return URes;
            }
        }

        context.logger.info("ShadowMemory(Local, {} - {})",
                            (void *)LaunchInfo.LocalShadowOffset,
                            (void *)LaunchInfo.LocalShadowOffsetEnd);
    } while (false);

    return UR_RESULT_SUCCESS;
}

std::vector<std::shared_ptr<USMAllocInfo>>
SanitizerInterceptor::findAllocInfoByAddress(uptr Address,
                                             ur_context_handle_t Context,
                                             ur_device_handle_t Device) {
    std::vector<std::shared_ptr<USMAllocInfo>> Result;
    auto current = std::make_shared<USMAllocInfo>(USMAllocInfo{Address});

    std::shared_lock<ur_shared_mutex> Guard(m_AllocationsMapMutex);

    auto It = std::lower_bound(
        m_AllocationsMap.begin(), m_AllocationsMap.end(), Address,
        [](const std::shared_ptr<USMAllocInfo> &AllocInfo, uptr Addr) {
            return (AllocInfo->AllocBegin + AllocInfo->AllocSize) < Addr;
        });
    for (; It != m_AllocationsMap.end(); ++It) {
        auto AI = *It;
        if (AI->AllocBegin > Address) {
            break;
        }
        if (Context && AI->Context != Context) {
            continue;
        }
        if (Device && AI->Device != Device) {
            continue;
        }
        Result.emplace_back(*It);
    }
    return Result;
}

LaunchInfo::~LaunchInfo() {
    if (LocalShadowOffset) {
        [[maybe_unused]] auto Result =
            context.urDdiTable.USM.pfnFree(Context, (void *)LocalShadowOffset);
        assert(Result == UR_RESULT_SUCCESS);
    }
}

} // namespace ur_sanitizer_layer
