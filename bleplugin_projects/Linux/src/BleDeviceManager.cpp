#include "BleDeviceManager.h"
#include <iostream>
#include <gio/gio.h>

// BlueZ D-Bus interface
#define BLUEZ_BUS_NAME "org.bluez"

BleDeviceManager& BleDeviceManager::GetInstance() {
    static BleDeviceManager instance;
    return instance;
}

bool BleDeviceManager::InitializeDBusConnection() {
    GError* error = nullptr;
    dbusConnection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }
    return true;
}

void BleDeviceManager::CleanupDBusConnection() {
    if (dbusConnection_) {
        g_object_unref(dbusConnection_);
        dbusConnection_ = nullptr;
    }
}

BleDeviceObject* BleDeviceManager::ConnectDevice(const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 既に接続済みの場合は既存のデバイスを返す
    auto it = connectedDevices_.find(address);
    if (it != connectedDevices_.end()) {
        return it->second.get();
    }

    // D-Bus接続を初期化
    if (!dbusConnection_ && !InitializeDBusConnection()) {
        return nullptr;
    }

    // デバイスパスを構築
    std::string devicePath = "/org/bluez/hci0/dev_" + address;
    std::replace(devicePath.begin(), devicePath.end(), ':', '_');

    // 新しいデバイスオブジェクトを作成
    auto device = std::make_unique<BleDeviceObject>(dbusConnection_, devicePath);
    if (!device->Connect()) {
        return nullptr;
    }

    BleDeviceObject* devicePtr = device.get();
    connectedDevices_[address] = std::move(device);
    return devicePtr;
}

void BleDeviceManager::DisconnectDevice(const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connectedDevices_.find(address);
    if (it != connectedDevices_.end()) {
        it->second->Disconnect();
        connectedDevices_.erase(it);
    }
}

void BleDeviceManager::DisconnectAllDevices() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& pair : connectedDevices_) {
        pair.second->Disconnect();
    }
    connectedDevices_.clear();
}

bool BleDeviceManager::IsDeviceConnected(const std::string& address) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connectedDevices_.find(address);
    return it != connectedDevices_.end() && it->second->IsConnected();
}

BleDeviceObject* BleDeviceManager::GetDeviceByAddress(const std::string& address) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = connectedDevices_.find(address);
    return it != connectedDevices_.end() ? it->second.get() : nullptr;
}

std::vector<BleDeviceObject*> BleDeviceManager::GetConnectedDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<BleDeviceObject*> devices;
    for (const auto& pair : connectedDevices_) {
        if (pair.second->IsConnected()) {
            devices.push_back(pair.second.get());
        }
    }
    return devices;
}

void BleDeviceManager::Update() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 切断されたデバイスを削除
    for (auto it = connectedDevices_.begin(); it != connectedDevices_.end();) {
        if (!it->second->IsConnected()) {
            it = connectedDevices_.erase(it);
        } else {
            ++it;
        }
    }
}
