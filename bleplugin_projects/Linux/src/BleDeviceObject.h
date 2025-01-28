#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <gio/gio.h>
#include "UuidManager.h"

class BleDeviceObject {
public:
    struct CharacteristicInfo {
        std::string path;
        UuidObject* serviceUuid;
        UuidObject* characteristicUuid;
    };

    BleDeviceObject(GDBusConnection* connection, const std::string& devicePath);
    ~BleDeviceObject();

    // デバイス情報
    std::string GetAddress() const;
    std::string GetName() const;
    bool IsConnected() const;
    int GetRssi() const;

    // 接続管理
    bool Connect();
    bool Disconnect();

    // サービス・キャラクタリスティック
    bool DiscoverServices();
    std::vector<CharacteristicInfo> GetCharacteristics() const;

    // 読み書き操作
    bool ReadCharacteristic(const std::string& servicePath, const std::string& characteristicPath, std::vector<uint8_t>& data);
    bool WriteCharacteristic(const std::string& servicePath, const std::string& characteristicPath, const std::vector<uint8_t>& data);

    // 通知設定
    bool EnableNotification(const std::string& servicePath, const std::string& characteristicPath, 
                          std::function<void(const std::vector<uint8_t>&)> callback);
    bool DisableNotification(const std::string& servicePath, const std::string& characteristicPath);

private:
    bool GetDeviceProxy();
    bool GetDeviceProperties();
    static void OnPropertiesChanged(GDBusProxy* proxy, GVariant* changed_properties,
                                  GStrv invalidated_properties, gpointer user_data);

    GDBusConnection* dbusConnection_;
    GDBusProxy* deviceProxy_;
    std::string devicePath_;
    std::string address_;
    std::string name_;
    bool isConnected_;
    int rssi_;
    std::vector<CharacteristicInfo> characteristics_;
    std::map<std::string, gulong> signalHandlers_;
    std::map<std::string, std::function<void(const std::vector<uint8_t>&)>> notificationCallbacks_;
};
