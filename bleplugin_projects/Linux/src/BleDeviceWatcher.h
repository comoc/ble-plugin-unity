#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <functional>
#include "UuidManager.h"

struct ScannedDeviceInfo {
    std::string address;
    std::string name;
    int rssi;
};

class BleDeviceWatcher {
public:
    static BleDeviceWatcher& GetInstance();

    // スキャン制御
    void AddScanFilter(UuidObject* serviceUuid);
    void ClearScanFilter();
    bool StartScan();
    bool StopScan();
    void Update();

    // スキャン結果
    std::vector<ScannedDeviceInfo> GetScannedDevices() const;

private:
    BleDeviceWatcher();
    ~BleDeviceWatcher() = default;
    BleDeviceWatcher(const BleDeviceWatcher&) = delete;
    BleDeviceWatcher& operator=(const BleDeviceWatcher&) = delete;

    void OnDeviceFound(const std::string& address, const std::string& name, int rssi);
    bool CheckServiceUuid(const std::vector<std::string>& deviceUuids);

    std::vector<UuidObject*> scanFilterUuids_;
    std::map<std::string, ScannedDeviceInfo> scannedDevices_;
    mutable std::mutex mutex_;
    bool isScanning_;
};
