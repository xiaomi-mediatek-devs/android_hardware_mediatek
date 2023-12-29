/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/power/IPower.h>
#include <aidl/google/hardware/power/extension/pixel/IPowerExt.h>
#include <android/hardware/thermal/2.0/IThermal.h>

#include <queue>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace android {
namespace hardware {
namespace thermal {
namespace V2_0 {
namespace implementation {

using ::aidl::android::hardware::power::IPower;
using ::aidl::google::hardware::power::extension::pixel::IPowerExt;

using ::android::hardware::hidl_vec;
using ::android::hardware::thermal::V2_0::IThermal;
using Temperature_2_0 = ::android::hardware::thermal::V2_0::Temperature;
using ::android::hardware::thermal::V2_0::TemperatureThreshold;
using ::android::hardware::thermal::V2_0::ThrottlingSeverity;
using CdevRequestStatus = std::unordered_map<std::string, int>;

class PowerHalService {
  public:
    PowerHalService();
    ~PowerHalService() = default;
    bool connect();
    bool isAidlPowerHalExist() { return power_hal_aidl_exist_; }
    bool isModeSupported(const std::string &type, const ThrottlingSeverity &t);
    bool isPowerHalConnected() { return power_hal_aidl_ != nullptr; }
    bool isPowerHalExtConnected() { return power_hal_ext_aidl_ != nullptr; }
    void setMode(const std::string &type, const ThrottlingSeverity &t, const bool &enable,
                 const bool error_on_exit = false);

  private:
    bool power_hal_aidl_exist_;
    std::shared_ptr<IPower> power_hal_aidl_;
    std::shared_ptr<IPowerExt> power_hal_ext_aidl_;
    std::mutex lock_;
};

}  // namespace implementation
}  // namespace V2_0
}  // namespace thermal
}  // namespace hardware
}  // namespace android
