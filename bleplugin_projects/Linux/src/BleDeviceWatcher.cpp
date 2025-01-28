#include "BleDeviceWatcher.h"
#include "BluetoothAdapterChecker.h"
#include <iostream>
#include <algorithm>
#include <gio/gio.h>

// BlueZ D-Bus interface
#define BLUEZ_BUS_NAME "org.bluez"
#define BLUEZ_INTERFACE_ADAPTER "org.bluez.Adapter1"
#define BLUEZ_INTERFACE_DEVICE "org.bluez.Device1"

BleDeviceWatcher& BleDeviceWatcher::GetInstance() {
    static BleDeviceWatcher instance;
    return instance;
}

BleDeviceWatcher::BleDeviceWatcher()
    : isScanning_(false) {
}

void BleDeviceWatcher::AddScanFilter(UuidObject* serviceUuid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (serviceUuid) {
        scanFilterUuids_.push_back(serviceUuid);
    }
}

void BleDeviceWatcher::ClearScanFilter() {
    std::lock_guard<std::mutex> lock(mutex_);
    scanFilterUuids_.clear();
}

bool BleDeviceWatcher::StartScan() {
    auto& adapter = BluetoothAdapterChecker::GetInstance();
    if (!adapter.IsAdapterAvailable() || !adapter.IsPowered()) {
        return false;
    }

    if (isScanning_) {
        return true;
    }

    if (!adapter.StartDiscovery()) {
        return false;
    }

    isScanning_ = true;
    return true;
}

bool BleDeviceWatcher::StopScan() {
    if (!isScanning_) {
        return true;
    }

    auto& adapter = BluetoothAdapterChecker::GetInstance();
    if (!adapter.StopDiscovery()) {
        return false;
    }

    isScanning_ = false;
    return true;
}

void BleDeviceWatcher::Update() {
    if (!isScanning_) {
        return;
    }

    // BlueZのD-Bus接続を取得
    GError* error = nullptr;
    GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return;
    }

    // ObjectManagerを取得
    GDBusProxy* objectManager = g_dbus_proxy_new_sync(
        connection,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        "/",
        "org.freedesktop.DBus.ObjectManager",
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to get ObjectManager: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    // ManagedObjectsを取得
    GVariant* managedObjects = g_dbus_proxy_call_sync(
        objectManager,
        "GetManagedObjects",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    g_object_unref(objectManager);

    if (error) {
        std::cerr << "Failed to get managed objects: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(connection);
        return;
    }

    // デバイスを探す
    GVariantIter* iter;
    const gchar* objectPath;
    GVariant* interfaces;

    g_variant_get(managedObjects, "(a{oa{sa{sv}}})", &iter);
    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &objectPath, &interfaces)) {
        GVariantIter interfaceIter;
        const gchar* interfaceName;
        GVariant* properties;

        g_variant_iter_init(&interfaceIter, interfaces);
        while (g_variant_iter_loop(&interfaceIter, "{&s@a{sv}}", &interfaceName, &properties)) {
            if (g_strcmp0(interfaceName, BLUEZ_INTERFACE_DEVICE) == 0) {
                std::string address;
                std::string name;
                int rssi = 0;
                std::vector<std::string> serviceUuids;

                // プロパティを取得
                GVariantIter propIter;
                const gchar* key;
                GVariant* value;

                g_variant_iter_init(&propIter, properties);
                while (g_variant_iter_loop(&propIter, "{&sv}", &key, &value)) {
                    if (g_strcmp0(key, "Address") == 0) {
                        address = g_variant_get_string(value, nullptr);
                    } else if (g_strcmp0(key, "Name") == 0) {
                        name = g_variant_get_string(value, nullptr);
                    } else if (g_strcmp0(key, "RSSI") == 0) {
                        rssi = g_variant_get_int16(value);
                    } else if (g_strcmp0(key, "UUIDs") == 0) {
                        GVariantIter* uuidIter;
                        const gchar* uuid;
                        g_variant_get(value, "as", &uuidIter);
                        while (g_variant_iter_loop(uuidIter, "&s", &uuid)) {
                            serviceUuids.push_back(uuid);
                        }
                        g_variant_iter_free(uuidIter);
                    }
                }

                // フィルタチェック
                if (!scanFilterUuids_.empty() && !CheckServiceUuid(serviceUuids)) {
                    continue;
                }

                // デバイス情報を更新
                OnDeviceFound(address, name, rssi);
            }
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(managedObjects);
    g_object_unref(connection);
}

std::vector<ScannedDeviceInfo> BleDeviceWatcher::GetScannedDevices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ScannedDeviceInfo> devices;
    for (const auto& pair : scannedDevices_) {
        devices.push_back(pair.second);
    }
    return devices;
}

void BleDeviceWatcher::OnDeviceFound(const std::string& address, const std::string& name, int rssi) {
    std::lock_guard<std::mutex> lock(mutex_);
    ScannedDeviceInfo& info = scannedDevices_[address];
    info.address = address;
    info.name = name;
    info.rssi = rssi;
}

bool BleDeviceWatcher::CheckServiceUuid(const std::vector<std::string>& deviceUuids) {
    for (const auto& filterUuid : scanFilterUuids_) {
        std::string filterUuidStr = filterUuid->GetUuidString();
        if (std::find(deviceUuids.begin(), deviceUuids.end(), filterUuidStr) != deviceUuids.end()) {
            return true;
        }
    }
    return false;
}
