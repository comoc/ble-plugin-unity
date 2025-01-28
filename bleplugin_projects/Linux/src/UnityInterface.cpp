#include "UnityInterface.h"
#include "BluetoothAdapterChecker.h"
#include "BleDeviceManager.h"
#include "BleDeviceWatcher.h"
#include "UuidManager.h"
#include <iostream>

extern "C" {

void _BlePluginBleAdapterStatusRequest() {
    BluetoothAdapterChecker::GetInstance().RequestAdapterStatus();
}

int _BlePluginBleAdapterUpdate() {
    BluetoothAdapterChecker::GetInstance().UpdateAdapterStatus();
    return BluetoothAdapterChecker::GetInstance().IsPowered() ? 1 : 0;
}

void _BlePluginFinalize() {
    BleDeviceManager::GetInstance().DisconnectAllDevices();
}

UuidHandle _BlePluginGetOrCreateUuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4) {
    return UuidManager::GetInstance().GetOrCreateUuidObject(d1, d2, d3, d4);
}

void _BlePluginConvertUuidUint128(UuidHandle ptr, void* out) {
    if (ptr && out) {
        static_cast<UuidObject*>(ptr)->GetUuid128(static_cast<uint8_t*>(out));
    }
}

void _BlePluginUpdateWatcher() {
    BleDeviceWatcher::GetInstance().Update();
}

void _BlePluginUpdateDevicdeManger() {
    BleDeviceManager::GetInstance().Update();
}

void _BlePluginAddScanServiceUuid(UuidHandle uuid) {
    if (uuid) {
        BleDeviceWatcher::GetInstance().AddScanFilter(static_cast<UuidObject*>(uuid));
    }
}

void _BlePluginStartScan() {
    BleDeviceWatcher::GetInstance().StartScan();
}

void _BlePluginStopScan() {
    BleDeviceWatcher::GetInstance().StopScan();
}

void _BlePluginClearScanFilter() {
    BleDeviceWatcher::GetInstance().ClearScanFilter();
}

int _BlePluginScanGetDeviceLength() {
    return static_cast<int>(BleDeviceWatcher::GetInstance().GetScannedDevices().size());
}

uint64_t _BlePluginScanGetDeviceAddr(int idx) {
    auto devices = BleDeviceWatcher::GetInstance().GetScannedDevices();
    if (idx >= 0 && idx < devices.size()) {
        // MACアドレスを64bit整数に変換
        uint64_t addr = 0;
        std::string addrStr = devices[idx].address;
        for (int i = 0; i < 6; i++) {
            addr = (addr << 8) | std::stoul(addrStr.substr(i * 3, 2), nullptr, 16);
        }
        return addr;
    }
    return 0;
}

const char* _BlePluginScanGetDeviceName(int idx) {
    auto devices = BleDeviceWatcher::GetInstance().GetScannedDevices();
    if (idx >= 0 && idx < devices.size()) {
        return devices[idx].name.c_str();
    }
    return "";
}

int32_t _BlePluginScanGetDeviceRssi(int idx) {
    auto devices = BleDeviceWatcher::GetInstance().GetScannedDevices();
    if (idx >= 0 && idx < devices.size()) {
        return devices[idx].rssi;
    }
    return 0;
}

DeviceHandle _BlePluginConnectDevice(uint64_t addr) {
    // 64bit整数からMACアドレス文字列に変換
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)((addr >> 40) & 0xFF),
             (unsigned int)((addr >> 32) & 0xFF),
             (unsigned int)((addr >> 24) & 0xFF),
             (unsigned int)((addr >> 16) & 0xFF),
             (unsigned int)((addr >> 8) & 0xFF),
             (unsigned int)(addr & 0xFF));

    return BleDeviceManager::GetInstance().ConnectDevice(addrStr);
}

void _BlePluginDisconnectDevice(uint64_t addr) {
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)((addr >> 40) & 0xFF),
             (unsigned int)((addr >> 32) & 0xFF),
             (unsigned int)((addr >> 24) & 0xFF),
             (unsigned int)((addr >> 16) & 0xFF),
             (unsigned int)((addr >> 8) & 0xFF),
             (unsigned int)(addr & 0xFF));

    BleDeviceManager::GetInstance().DisconnectDevice(addrStr);
}

void _BlePluginDisconnectAllDevice() {
    BleDeviceManager::GetInstance().DisconnectAllDevices();
}

bool _BlePluginIsDeviceConnectedByAddr(uint64_t addr) {
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)((addr >> 40) & 0xFF),
             (unsigned int)((addr >> 32) & 0xFF),
             (unsigned int)((addr >> 24) & 0xFF),
             (unsigned int)((addr >> 16) & 0xFF),
             (unsigned int)((addr >> 8) & 0xFF),
             (unsigned int)(addr & 0xFF));

    return BleDeviceManager::GetInstance().IsDeviceConnected(addrStr);
}

bool _BlePluginIsDeviceConnected(DeviceHandle devicePtr) {
    if (auto device = static_cast<BleDeviceObject*>(devicePtr)) {
        return device->IsConnected();
    }
    return false;
}

uint64_t _BlePluginDeviceGetAddr(DeviceHandle devicePtr) {
    if (auto device = static_cast<BleDeviceObject*>(devicePtr)) {
        std::string addrStr = device->GetAddress();
        uint64_t addr = 0;
        for (int i = 0; i < 6; i++) {
            addr = (addr << 8) | std::stoul(addrStr.substr(i * 3, 2), nullptr, 16);
        }
        return addr;
    }
    return 0;
}

int _BlePluginGetConectDeviceNum() {
    return static_cast<int>(BleDeviceManager::GetInstance().GetConnectedDevices().size());
}

uint64_t _BlePluginGetConectDevicAddr(int idx) {
    auto devices = BleDeviceManager::GetInstance().GetConnectedDevices();
    if (idx >= 0 && idx < devices.size()) {
        std::string addrStr = devices[idx]->GetAddress();
        uint64_t addr = 0;
        for (int i = 0; i < 6; i++) {
            addr = (addr << 8) | std::stoul(addrStr.substr(i * 3, 2), nullptr, 16);
        }
        return addr;
    }
    return 0;
}

DeviceHandle _BlePluginGetConnectDevicePtr(int idx) {
    auto devices = BleDeviceManager::GetInstance().GetConnectedDevices();
    if (idx >= 0 && idx < devices.size()) {
        return devices[idx];
    }
    return nullptr;
}

DeviceHandle _BlePluginGetDevicePtrByAddr(uint64_t addr) {
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned int)((addr >> 40) & 0xFF),
             (unsigned int)((addr >> 32) & 0xFF),
             (unsigned int)((addr >> 24) & 0xFF),
             (unsigned int)((addr >> 16) & 0xFF),
             (unsigned int)((addr >> 8) & 0xFF),
             (unsigned int)(addr & 0xFF));

    return BleDeviceManager::GetInstance().GetDeviceByAddress(addrStr);
}

int _BlePluginDeviceCharastricsNum(DeviceHandle devicePtr) {
    if (auto device = static_cast<BleDeviceObject*>(devicePtr)) {
        return static_cast<int>(device->GetCharacteristics().size());
    }
    return 0;
}

UuidHandle _BlePluginDeviceCharastricUuid(DeviceHandle devicePtr, int idx) {
    if (auto device = static_cast<BleDeviceObject*>(devicePtr)) {
        auto characteristics = device->GetCharacteristics();
        if (idx >= 0 && idx < characteristics.size()) {
            return characteristics[idx].characteristicUuid;
        }
    }
    return nullptr;
}

UuidHandle _BlePluginDeviceCharastricServiceUuid(DeviceHandle devicePtr, int idx) {
    if (auto device = static_cast<BleDeviceObject*>(devicePtr)) {
        auto characteristics = device->GetCharacteristics();
        if (idx >= 0 && idx < characteristics.size()) {
            return characteristics[idx].serviceUuid;
        }
    }
    return nullptr;
}

ReadRequestHandle _BlePluginReadCharacteristicRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid) {
    // TODO: 読み取りリクエストの実装
    return nullptr;
}

WriteRequestHandle _BlePluginWriteCharacteristicRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid, void* data, int size) {
    // TODO: 書き込みリクエストの実装
    return nullptr;
}

bool _BlePluginIsReadRequestComplete(ReadRequestHandle ptr) {
    // TODO: 読み取りリクエストの完了確認の実装
    return false;
}

bool _BlePluginIsReadRequestError(ReadRequestHandle ptr) {
    // TODO: 読み取りリクエストのエラー確認の実装
    return false;
}

int _BlePluginCopyReadRequestData(ReadRequestHandle ptr, void* data, int maxSize) {
    // TODO: 読み取りデータのコピー実装
    return 0;
}

void _BlePluginReleaseReadRequest(uint64_t deviceaddr, ReadRequestHandle ptr) {
    // TODO: 読み取りリクエストの解放実装
}

bool _BlePluginIsWriteRequestComplete(WriteRequestHandle ptr) {
    // TODO: 書き込みリクエストの完了確認の実装
    return false;
}

bool _BlePluginIsWriteRequestError(WriteRequestHandle ptr) {
    // TODO: 書き込みリクエストのエラー確認の実装
    return false;
}

void _BlePluginReleaseWriteRequest(uint64_t deviceaddr, WriteRequestHandle ptr) {
    // TODO: 書き込みリクエストの解放実装
}

void _BlePluginSetNotificateRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid, bool enable) {
    // TODO: 通知設定の実装
}

int _BlePluginGetDeviceNotificateNum(uint64_t addr) {
    // TODO: 通知数の取得実装
    return 0;
}

int _BlePluginCopyDeviceNotificateData(uint64_t addr, int idx, void* ptr, int maxSize) {
    // TODO: 通知データのコピー実装
    return 0;
}

UuidHandle _BlePluginGetDeviceNotificateServiceUuid(uint64_t addr, int idx) {
    // TODO: 通知サービスUUIDの取得実装
    return nullptr;
}

UuidHandle _BlePluginGetDeviceNotificateCharastricsUuid(uint64_t addr, int idx) {
    // TODO: 通知キャラクタリスティックUUIDの取得実装
    return nullptr;
}

} // extern "C"
