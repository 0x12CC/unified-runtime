/*
 *
 * Copyright (C) 2024 Intel Corporation
 *
 * Part of the Unified-Runtime Project, under the Apache License v2.0 with LLVM Exceptions.
 * See LICENSE.TXT
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 * @file asan_statistics.cpp
 *
 */

#include "asan_statistics.hpp"
#include "asan_interceptor.hpp"
#include "ur_sanitizer_layer.hpp"

#include <atomic>

namespace ur_sanitizer_layer {

struct AsanStats {
    void UpdateUSMMalloced(uptr MallocedSize, uptr RedzoneSize);
    void UpdateUSMFreed(uptr FreedSize);
    void UpdateUSMRealFreed(uptr FreedSize, uptr RedzoneSize);

    void UpdateShadowMmaped(uptr ShadowSize);
    void UpdateShadowMalloced(uptr ShadowSize);
    void UpdateShadowFreed(uptr ShadowSize);

    void Print(ur_context_handle_t Context);

  private:
    std::atomic<uptr> UsmMalloced;
    std::atomic<uptr> UsmMallocedRedzones;

    // Quarantined memory
    std::atomic<uptr> UsmFreed;

    std::atomic<uptr> ShadowMmaped;
    std::atomic<uptr> ShadowMalloced;

    double Overhead = 0.0;

    void UpdateOverhead();
};

void AsanStats::Print(ur_context_handle_t Context) {
    getContext()->logger.always("Stats: Context {}", (void *)Context);
    getContext()->logger.always("Stats:   peak memory overhead: {}%",
                                Overhead * 100);
}

void AsanStats::UpdateUSMMalloced(uptr MallocedSize, uptr RedzoneSize) {
    UsmMalloced += MallocedSize;
    UsmMallocedRedzones += RedzoneSize;
    getContext()->logger.debug(
        "Stats: UpdateUSMMalloced(UsmMalloced={}, UsmMallocedRedzones={})",
        UsmMalloced, UsmMallocedRedzones);
    UpdateOverhead();
}

void AsanStats::UpdateUSMFreed(uptr FreedSize) {
    UsmFreed += FreedSize;
    getContext()->logger.debug("Stats: UpdateUSMFreed(UsmFreed={})", UsmFreed);
}

void AsanStats::UpdateUSMRealFreed(uptr FreedSize, uptr RedzoneSize) {
    UsmMalloced -= FreedSize;
    UsmMallocedRedzones -= RedzoneSize;
    if (getContext()->interceptor->getOptions().MaxQuarantineSizeMB) {
        UsmFreed -= FreedSize;
    }
    getContext()->logger.debug(
        "Stats: UpdateUSMRealFreed(UsmMalloced={}, UsmMallocedRedzones={})",
        UsmMalloced, UsmMallocedRedzones);
    UpdateOverhead();
}

void AsanStats::UpdateShadowMmaped(uptr ShadowSize) {
    ShadowMmaped += ShadowSize;
    getContext()->logger.debug("Stats: UpdateShadowMmaped(ShadowMmaped={})",
                               ShadowMmaped);
    UpdateOverhead();
}

void AsanStats::UpdateShadowMalloced(uptr ShadowSize) {
    ShadowMalloced += ShadowSize;
    getContext()->logger.debug("Stats: UpdateShadowMalloced(ShadowMalloced={})",
                               ShadowMalloced);
    UpdateOverhead();
}

void AsanStats::UpdateShadowFreed(uptr ShadowSize) {
    ShadowMalloced -= ShadowSize;
    getContext()->logger.debug("Stats: UpdateShadowFreed(ShadowMalloced={})",
                               ShadowMalloced);
    UpdateOverhead();
}

void AsanStats::UpdateOverhead() {
    auto ShadowSize = ShadowMmaped + ShadowMalloced;
    auto TotalSize = UsmMalloced + ShadowSize;
    if (TotalSize == 0) {
        return;
    }
    auto NewOverhead = (ShadowSize + UsmMallocedRedzones) / (double)TotalSize;
    Overhead = std::max(Overhead, NewOverhead);
}

void AsanStatsWrapper::UpdateUSMMalloced(uptr MallocedSize, uptr RedzoneSize) {
    if (Stat) {
        Stat->UpdateUSMMalloced(MallocedSize, RedzoneSize);
    }
}

void AsanStatsWrapper::UpdateUSMFreed(uptr FreedSize) {
    if (Stat) {
        Stat->UpdateUSMFreed(FreedSize);
    }
}

void AsanStatsWrapper::UpdateUSMRealFreed(uptr FreedSize, uptr RedzoneSize) {
    if (Stat) {
        Stat->UpdateUSMRealFreed(FreedSize, RedzoneSize);
    }
}

void AsanStatsWrapper::UpdateShadowMmaped(uptr ShadowSize) {
    if (Stat) {
        Stat->UpdateShadowMmaped(ShadowSize);
    }
}

void AsanStatsWrapper::UpdateShadowMalloced(uptr ShadowSize) {
    if (Stat) {
        Stat->UpdateShadowMalloced(ShadowSize);
    }
}

void AsanStatsWrapper::UpdateShadowFreed(uptr ShadowSize) {
    if (Stat) {
        Stat->UpdateShadowFreed(ShadowSize);
    }
}

void AsanStatsWrapper::Print(ur_context_handle_t Context) {
    if (Stat) {
        Stat->Print(Context);
    }
}

AsanStatsWrapper::AsanStatsWrapper() : Stat(nullptr) {
    if (getContext()->interceptor->getOptions().PrintStats) {
        Stat = new AsanStats;
    }
}

AsanStatsWrapper::~AsanStatsWrapper() { delete Stat; }

} // namespace ur_sanitizer_layer