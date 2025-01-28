#include "BluetoothAdapterChecker.h"
#include <stdexcept>
#include <iostream>

// BlueZ D-Bus interface
#define BLUEZ_BUS_NAME "org.bluez"
#define BLUEZ_INTERFACE_ADAPTER "org.bluez.Adapter1"
#define BLUEZ_INTERFACE_OBJECT_MANAGER "org.freedesktop.DBus.ObjectManager"
#define BLUEZ_PATH "/org/bluez"

BluetoothAdapterChecker& BluetoothAdapterChecker::GetInstance() {
    static BluetoothAdapterChecker instance;
    return instance;
}

BluetoothAdapterChecker::BluetoothAdapterChecker()
    : dbusConnection_(nullptr)
    , adapter_(nullptr)
    , isInitialized_(false) {
    InitializeDBusConnection();
}

BluetoothAdapterChecker::~BluetoothAdapterChecker() {
    CleanupDBusConnection();
}

bool BluetoothAdapterChecker::InitializeDBusConnection() {
    GError* error = nullptr;

    // D-Bus システムバスに接続
    dbusConnection_ = g_bus_get_sync(G_BUS_TYPE_SYSTEM, nullptr, &error);
    if (error) {
        std::cerr << "Failed to connect to D-Bus: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    if (!GetBlueZAdapter()) {
        return false;
    }

    isInitialized_ = true;
    return true;
}

void BluetoothAdapterChecker::CleanupDBusConnection() {
    if (adapter_) {
        g_object_unref(adapter_);
        adapter_ = nullptr;
    }
    if (dbusConnection_) {
        g_object_unref(dbusConnection_);
        dbusConnection_ = nullptr;
    }
    isInitialized_ = false;
}

bool BluetoothAdapterChecker::GetBlueZAdapter() {
    GError* error = nullptr;

    // ObjectManagerを取得
    GDBusProxy* objectManager = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        BLUEZ_PATH,
        BLUEZ_INTERFACE_OBJECT_MANAGER,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to get ObjectManager: " << error->message << std::endl;
        g_error_free(error);
        return false;
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
        return false;
    }

    // アダプタを探す
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
            if (g_strcmp0(interfaceName, BLUEZ_INTERFACE_ADAPTER) == 0) {
                adapterPath_ = objectPath;
                break;
            }
        }
        if (!adapterPath_.empty()) {
            break;
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(managedObjects);

    if (adapterPath_.empty()) {
        std::cerr << "No Bluetooth adapter found" << std::endl;
        return false;
    }

    // アダプタプロキシを作成
    adapter_ = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        adapterPath_.c_str(),
        BLUEZ_INTERFACE_ADAPTER,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create adapter proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}

bool BluetoothAdapterChecker::IsAdapterAvailable() const {
    return isInitialized_ && adapter_ != nullptr;
}

bool BluetoothAdapterChecker::IsPowered() const {
    if (!IsAdapterAvailable()) {
        return false;
    }

    GVariant* powered = g_dbus_proxy_get_cached_property(adapter_, "Powered");
    if (!powered) {
        return false;
    }

    gboolean value = g_variant_get_boolean(powered);
    g_variant_unref(powered);
    return value;
}

bool BluetoothAdapterChecker::IsDiscovering() const {
    if (!IsAdapterAvailable()) {
        return false;
    }

    GVariant* discovering = g_dbus_proxy_get_cached_property(adapter_, "Discovering");
    if (!discovering) {
        return false;
    }

    gboolean value = g_variant_get_boolean(discovering);
    g_variant_unref(discovering);
    return value;
}

void BluetoothAdapterChecker::RequestAdapterStatus() {
    if (!isInitialized_) {
        InitializeDBusConnection();
    }
}

void BluetoothAdapterChecker::UpdateAdapterStatus() {
    if (!isInitialized_) {
        InitializeDBusConnection();
    } else if (!adapter_) {
        GetBlueZAdapter();
    }
}

bool BluetoothAdapterChecker::StartDiscovery() {
    if (!IsAdapterAvailable() || !IsPowered()) {
        return false;
    }

    GError* error = nullptr;
    g_dbus_proxy_call_sync(
        adapter_,
        "StartDiscovery",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to start discovery: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}

bool BluetoothAdapterChecker::StopDiscovery() {
    if (!IsAdapterAvailable()) {
        return false;
    }

    GError* error = nullptr;
    g_dbus_proxy_call_sync(
        adapter_,
        "StopDiscovery",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to stop discovery: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}
