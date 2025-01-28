#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include "BleDeviceObject.h"
#include "UuidManager.h"

class BleDeviceManager {
public:
    static BleDeviceManager& GetInstance();

    // デバイス接続管理
    BleDeviceObject* ConnectDevice(const std::string& address);
    void DisconnectDevice(const std::string& address);
    void DisconnectAllDevices();
    bool IsDeviceConnected(const std::string& address) const;
    BleDeviceObject* GetDeviceByAddress(const std::string& address);

    // 接続デバイス一覧
    std::vector<BleDeviceObject*> GetConnectedDevices() const;
    void Update();

private:
    BleDeviceManager() = default;
    ~BleDeviceManager() = default;
    BleDeviceManager(const BleDeviceManager&) = delete;
    BleDeviceManager& operator=(const BleDeviceManager&) = delete;

    std::map<std::string, std::unique_ptr<BleDeviceObject>> connectedDevices_;
    mutable std::mutex mutex_;
    GDBusConnection* dbusConnection_;

    bool InitializeDBusConnection();
    void CleanupDBusConnection();
};
