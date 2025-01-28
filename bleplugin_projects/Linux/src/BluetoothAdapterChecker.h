#pragma once
#include <string>
#include <memory>
#include <gio/gio.h>

class BluetoothAdapterChecker {
public:
    static BluetoothAdapterChecker& GetInstance();

    // アダプタの状態を確認
    bool IsAdapterAvailable() const;
    bool IsPowered() const;
    bool IsDiscovering() const;

    // アダプタの制御
    void RequestAdapterStatus();
    void UpdateAdapterStatus();
    bool StartDiscovery();
    bool StopDiscovery();

private:
    BluetoothAdapterChecker();
    ~BluetoothAdapterChecker();
    BluetoothAdapterChecker(const BluetoothAdapterChecker&) = delete;
    BluetoothAdapterChecker& operator=(const BluetoothAdapterChecker&) = delete;

    // D-Bus接続の初期化
    bool InitializeDBusConnection();
    void CleanupDBusConnection();

    // BlueZオブジェクトの取得
    bool GetBlueZAdapter();

    GDBusConnection* dbusConnection_;
    GDBusProxy* adapter_;
    std::string adapterPath_;
    bool isInitialized_;
};
