/*
 * Copyright (C) 2019 The LineageOS Project
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

#define LOG_TAG "FingerprintInscreenService"

#include "FingerprintInscreen.h"
#include <android-base/logging.h>
#include <hidl/HidlTransportSupport.h>
#include <fstream>

#define FINGERPRINT_ACQUIRED_VENDOR 6
#define FINGERPRINT_ERROR_VENDOR 8

#define OP_ENABLE_FP_LONGPRESS 3
#define OP_DISABLE_FP_LONGPRESS 4
#define OP_RESUME_FP_ENROLL 8
#define OP_FINISH_FP_ENROLL 10

#define OP_DISPLAY_AOD_MODE 8
#define OP_DISPLAY_NOTIFY_PRESS 9
#define OP_DISPLAY_SET_DIM 10

namespace vendor {
namespace lineage {
namespace biometrics {
namespace fingerprint {
namespace inscreen {
namespace V1_0 {
namespace implementation {

constexpr std::array<std::pair<uint32_t, uint32_t>, 21> BRIGHTNESS_ALPHA_LUT = {
    std::make_pair(0, 0xff),
    std::make_pair(1, 0xf1),
    std::make_pair(2, 0xec),
    std::make_pair(4, 0xeb),
    std::make_pair(5, 0xea),
    std::make_pair(6, 0xe8),
    std::make_pair(10, 0xe4),
    std::make_pair(20, 0xdc),
    std::make_pair(30, 0xd4),
    std::make_pair(45, 0xcc),
    std::make_pair(70, 0xbe),
    std::make_pair(100, 0xb3),
    std::make_pair(150, 0xa6),
    std::make_pair(227, 0x90),
    std::make_pair(300, 0x83),
    std::make_pair(400, 0x70),
    std::make_pair(500, 0x60),
    std::make_pair(600, 0x53),
    std::make_pair(800, 0x3c),
    std::make_pair(1023, 0x22),
    std::make_pair(2000, 0x83),
};

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

int interpolate(int x, int xa, int xb, int ya, int yb) {
    int bf = 2 * (yb - ya) * (x - xa) / (xb - xa);
    int factor = bf / 2;
    int plus = bf % 2;
    int sub = 0;

    if ((xa - xb) && (yb - ya)) {
        sub = 2 * (x - xa) * (x - xb) / (yb - ya) / (xa - xb);
    }

    return ya + factor + plus + sub;
}

int brightness_to_alpha(int brightness) {
    int i = 0;

    for (; i < BRIGHTNESS_ALPHA_LUT.size(); i++) {
        if (BRIGHTNESS_ALPHA_LUT[i].first >= brightness) {
            break;
        }
    }

    if (i == 0)
        return BRIGHTNESS_ALPHA_LUT.front().second;

    if (i == BRIGHTNESS_ALPHA_LUT.size())
        return BRIGHTNESS_ALPHA_LUT.back().second;

    return interpolate(brightness,
            BRIGHTNESS_ALPHA_LUT[i - 1].first,
            BRIGHTNESS_ALPHA_LUT[i].first,
            BRIGHTNESS_ALPHA_LUT[i - 1].second,
            BRIGHTNESS_ALPHA_LUT[i].second);
}

FingerprintInscreen::FingerprintInscreen() {
    this->mFodCircleVisible = false;
    this->mVendorFpService = IVendorFingerprintExtensions::getService();
    this->mVendorDisplayService = IOneplusDisplay::getService();
}

Return<void> FingerprintInscreen::onStartEnroll() {
    this->mVendorFpService->updateStatus(OP_DISABLE_FP_LONGPRESS);
    this->mVendorFpService->updateStatus(OP_RESUME_FP_ENROLL);

    return Void();
}

Return<void> FingerprintInscreen::onFinishEnroll() {
    this->mVendorFpService->updateStatus(OP_FINISH_FP_ENROLL);

    return Void();
}

Return<void> FingerprintInscreen::onPress() {
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 1);

    return Void();
}

Return<void> FingerprintInscreen::onRelease() {
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 0);

    return Void();
}

Return<void> FingerprintInscreen::onShowFODView() {
    this->mFodCircleVisible = true;
    this->mVendorDisplayService->setMode(OP_DISPLAY_SET_DIM, 1);

    return Void();
}

Return<void> FingerprintInscreen::onHideFODView() {
    this->mFodCircleVisible = false;
    this->mVendorDisplayService->setMode(OP_DISPLAY_SET_DIM, 0);
    this->mVendorDisplayService->setMode(OP_DISPLAY_NOTIFY_PRESS, 0);

    return Void();
}

Return<bool> FingerprintInscreen::handleAcquired(int32_t acquiredInfo, int32_t vendorCode) {
    std::lock_guard<std::mutex> _lock(mCallbackLock);
    if (mCallback == nullptr) {
        return false;
    }

    if (acquiredInfo == FINGERPRINT_ACQUIRED_VENDOR) {
        if (mFodCircleVisible && vendorCode == 0) {
            Return<void> ret = mCallback->onFingerDown();
            if (!ret.isOk()) {
                LOG(ERROR) << "FingerDown() error: " << ret.description();
            }
            return true;
        }

        if (mFodCircleVisible && vendorCode == 1) {
            Return<void> ret = mCallback->onFingerUp();
            if (!ret.isOk()) {
                LOG(ERROR) << "FingerUp() error: " << ret.description();
            }
            return true;
        }
    }

    return false;
}

Return<bool> FingerprintInscreen::handleError(int32_t error, int32_t vendorCode) {
    return error == FINGERPRINT_ERROR_VENDOR && vendorCode == 6;
}

Return<void> FingerprintInscreen::setLongPressEnabled(bool enabled) {
    this->mVendorFpService->updateStatus(
            enabled ? OP_ENABLE_FP_LONGPRESS : OP_DISABLE_FP_LONGPRESS);

    return Void();
}

Return<int32_t> FingerprintInscreen::getDimAmount(int32_t brightness) {
    int realBrightness = brightness * 1023 / 255;
    int dimAmount = (brightness_to_alpha(realBrightness) * 70) / 100;

    LOG(INFO) << "dimAmount = " << dimAmount;

    return dimAmount;
}

Return<bool> FingerprintInscreen::shouldBoostBrightness() {
    return false;
}

Return<void> FingerprintInscreen::setCallback(const sp<IFingerprintInscreenCallback>& callback) {
    {
        std::lock_guard<std::mutex> _lock(mCallbackLock);
        mCallback = callback;
    }

    return Void();
}

Return<int32_t> FingerprintInscreen::getPositionX() {
    return 444;
}

Return<int32_t> FingerprintInscreen::getPositionY() {
    return 1966;
}

Return<int32_t> FingerprintInscreen::getSize() {
    return 190;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace inscreen
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace lineage
}  // namespace vendor
