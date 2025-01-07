/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Memtrack.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

namespace aidl::android::hardware::memtrack {

bool getIonMemory(int processId, int64_t* memSize) {
    FILE* memFile = fopen("/proc/ion/clients/clients_summary", "r");
    if (!memFile) return false;

    *memSize = 0;
    char fileLine[1024];
    bool isFound = false;
    while (fgets(fileLine, sizeof(fileLine), memFile)) {
        int clientId;
        int64_t allocSize;
        if (sscanf(fileLine, "%*s %d %ld", &clientId, &allocSize) == 2 && processId == clientId) {
            *memSize += allocSize;
            isFound = true;
        }
    }
    fclose(memFile);
    return isFound;
}

bool getMaliGpuMemory(int processId, int64_t* memSize) {
    FILE* memFile = fopen("/proc/mtk_mali/gpu_memory", "r");
    if (!memFile) return false;

    char fileLine[1024];
    while (fgets(fileLine, sizeof(fileLine), memFile)) {
        if (fileLine[0] == ' ' && fileLine[1] == ' ') {
            int64_t gpuAlloc;
            unsigned int clientId;
            if (sscanf(fileLine, "  %*s %ld %u", &gpuAlloc, &clientId) == 2) {
                if (processId == clientId || processId == 0) {
                    *memSize = gpuAlloc * PAGE_SIZE;
                    if (processId != 0) break;
                }
            }
        }
    }
    fclose(memFile);
    return true;
}

bool getGraphicsMemory(int processId, int64_t* memSize) {
    return getIonMemory(processId, memSize);
}

bool getGlMemory(int processId, int64_t* memSize) {
    return getMaliGpuMemory(processId, memSize);
}

ndk::ScopedAStatus Memtrack::getMemory(int processId, MemtrackType memType,
                                       std::vector<MemtrackRecord>* records) {
    if (processId < 0) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_ILLEGAL_ARGUMENT));
    if (memType != MemtrackType::OTHER && memType != MemtrackType::GL &&
        memType != MemtrackType::GRAPHICS && memType != MemtrackType::MULTIMEDIA &&
        memType != MemtrackType::CAMERA) {
        return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_UNSUPPORTED_OPERATION));
    }
    if (!records) return ndk::ScopedAStatus(AStatus_fromExceptionCode(EX_NULL_POINTER));

    records->clear();
    int64_t totalSize = 0;

    if ((memType == MemtrackType::GL && getGlMemory(processId, &totalSize)) ||
        (memType == MemtrackType::GRAPHICS && getGraphicsMemory(processId, &totalSize))) {
        MemtrackRecord memRecord = {
                .flags = memType == MemtrackType::GL
                                 ? (MemtrackRecord::FLAG_SMAPS_UNACCOUNTED |
                                    MemtrackRecord::FLAG_PRIVATE | MemtrackRecord::FLAG_NONSECURE)
                                 : (MemtrackRecord::FLAG_SMAPS_UNACCOUNTED |
                                    MemtrackRecord::FLAG_SHARED | MemtrackRecord::FLAG_SYSTEM |
                                    MemtrackRecord::FLAG_NONSECURE),
                .sizeInBytes = totalSize};
        records->push_back(memRecord);
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Memtrack::getGpuDeviceInfo(std::vector<DeviceInfo>* deviceList) {
    const std::string sysfsPath = "/sys/class/misc/mali0/device";
    std::string devName = "mali_gpu";

    struct stat statbuf;
    if (lstat(sysfsPath.c_str(), &statbuf) == 0 && S_ISLNK(statbuf.st_mode)) {
        char buffer[PATH_MAX];
        ssize_t len = readlink(sysfsPath.c_str(), buffer, sizeof(buffer) - 1);
        if (len != -1) {
            buffer[len] = '\0';
            devName = std::string(buffer);
        }
    }

    deviceList->clear();
    deviceList->emplace_back(DeviceInfo{.id = 0, .name = devName});
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::memtrack
