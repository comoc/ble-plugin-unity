#include "BleDeviceObject.h"
#include <iostream>

// BlueZ D-Bus interface
#define BLUEZ_BUS_NAME "org.bluez"
#define BLUEZ_INTERFACE_DEVICE "org.bluez.Device1"
#define BLUEZ_INTERFACE_GATT_SERVICE "org.bluez.GattService1"
#define BLUEZ_INTERFACE_GATT_CHARACTERISTIC "org.bluez.GattCharacteristic1"

BleDeviceObject::BleDeviceObject(GDBusConnection* connection, const std::string& devicePath)
    : dbusConnection_(connection)
    , deviceProxy_(nullptr)
    , devicePath_(devicePath)
    , isConnected_(false)
    , rssi_(0) {
    GetDeviceProxy();
    GetDeviceProperties();
}

BleDeviceObject::~BleDeviceObject() {
    // シグナルハンドラの解除
    for (const auto& handler : signalHandlers_) {
        g_signal_handler_disconnect(deviceProxy_, handler.second);
    }
    signalHandlers_.clear();

    if (deviceProxy_) {
        g_object_unref(deviceProxy_);
    }
}

bool BleDeviceObject::GetDeviceProxy() {
    GError* error = nullptr;
    deviceProxy_ = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        devicePath_.c_str(),
        BLUEZ_INTERFACE_DEVICE,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create device proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    // プロパティ変更通知のシグナルハンドラを設定
    gulong handler = g_signal_connect(
        deviceProxy_,
        "g-properties-changed",
        G_CALLBACK(OnPropertiesChanged),
        this
    );
    signalHandlers_["properties-changed"] = handler;

    return true;
}

bool BleDeviceObject::GetDeviceProperties() {
    if (!deviceProxy_) {
        return false;
    }

    GVariant* addr = g_dbus_proxy_get_cached_property(deviceProxy_, "Address");
    if (addr) {
        address_ = g_variant_get_string(addr, nullptr);
        g_variant_unref(addr);
    }

    GVariant* name = g_dbus_proxy_get_cached_property(deviceProxy_, "Name");
    if (name) {
        name_ = g_variant_get_string(name, nullptr);
        g_variant_unref(name);
    }

    GVariant* connected = g_dbus_proxy_get_cached_property(deviceProxy_, "Connected");
    if (connected) {
        isConnected_ = g_variant_get_boolean(connected);
        g_variant_unref(connected);
    }

    GVariant* rssi = g_dbus_proxy_get_cached_property(deviceProxy_, "RSSI");
    if (rssi) {
        rssi_ = g_variant_get_int16(rssi);
        g_variant_unref(rssi);
    }

    return true;
}

void BleDeviceObject::OnPropertiesChanged(GDBusProxy* proxy, GVariant* changed_properties,
                                        GStrv invalidated_properties, gpointer user_data) {
    BleDeviceObject* device = static_cast<BleDeviceObject*>(user_data);
    device->GetDeviceProperties();
}

std::string BleDeviceObject::GetAddress() const {
    return address_;
}

std::string BleDeviceObject::GetName() const {
    return name_;
}

bool BleDeviceObject::IsConnected() const {
    return isConnected_;
}

int BleDeviceObject::GetRssi() const {
    return rssi_;
}

bool BleDeviceObject::Connect() {
    if (!deviceProxy_) {
        return false;
    }

    GError* error = nullptr;
    g_dbus_proxy_call_sync(
        deviceProxy_,
        "Connect",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to connect: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}

bool BleDeviceObject::Disconnect() {
    if (!deviceProxy_) {
        return false;
    }

    GError* error = nullptr;
    g_dbus_proxy_call_sync(
        deviceProxy_,
        "Disconnect",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to disconnect: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}

bool BleDeviceObject::DiscoverServices() {
    if (!deviceProxy_ || !isConnected_) {
        return false;
    }

    characteristics_.clear();

    // ObjectManagerを取得
    GError* error = nullptr;
    GDBusProxy* objectManager = g_dbus_proxy_new_sync(
        dbusConnection_,
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

    // サービスとキャラクタリスティックを探す
    GVariantIter* iter;
    const gchar* objectPath;
    GVariant* interfaces;

    g_variant_get(managedObjects, "(a{oa{sa{sv}}})", &iter);
    while (g_variant_iter_loop(iter, "{&o@a{sa{sv}}}", &objectPath, &interfaces)) {
        std::string path(objectPath);
        if (path.find(devicePath_) != 0) {
            continue;
        }

        GVariantIter interfaceIter;
        const gchar* interfaceName;
        GVariant* properties;

        g_variant_iter_init(&interfaceIter, interfaces);
        while (g_variant_iter_loop(&interfaceIter, "{&s@a{sv}}", &interfaceName, &properties)) {
            if (g_strcmp0(interfaceName, BLUEZ_INTERFACE_GATT_CHARACTERISTIC) == 0) {
                CharacteristicInfo info;
                info.path = path;

                // UUIDを取得
                GVariantIter propIter;
                const gchar* key;
                GVariant* value;

                g_variant_iter_init(&propIter, properties);
                while (g_variant_iter_loop(&propIter, "{&sv}", &key, &value)) {
                    if (g_strcmp0(key, "UUID") == 0) {
                        const gchar* uuid = g_variant_get_string(value, nullptr);
                        // TODO: UUIDの変換処理
                    } else if (g_strcmp0(key, "Service") == 0) {
                        const gchar* service = g_variant_get_string(value, nullptr);
                        // TODO: サービスUUIDの取得
                    }
                }

                characteristics_.push_back(info);
            }
        }
    }

    g_variant_iter_free(iter);
    g_variant_unref(managedObjects);

    return true;
}

std::vector<BleDeviceObject::CharacteristicInfo> BleDeviceObject::GetCharacteristics() const {
    return characteristics_;
}

bool BleDeviceObject::ReadCharacteristic(const std::string& servicePath, const std::string& characteristicPath,
                                       std::vector<uint8_t>& data) {
    GError* error = nullptr;
    GDBusProxy* charaProxy = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        characteristicPath.c_str(),
        BLUEZ_INTERFACE_GATT_CHARACTERISTIC,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create characteristic proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    GVariant* result = g_dbus_proxy_call_sync(
        charaProxy,
        "ReadValue",
        g_variant_new("(a{sv})", nullptr),
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    g_object_unref(charaProxy);

    if (error) {
        std::cerr << "Failed to read characteristic: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    GVariantIter* iter;
    g_variant_get(result, "(ay)", &iter);

    data.clear();
    guint8 value;
    while (g_variant_iter_loop(iter, "y", &value)) {
        data.push_back(value);
    }

    g_variant_iter_free(iter);
    g_variant_unref(result);

    return true;
}

bool BleDeviceObject::WriteCharacteristic(const std::string& servicePath, const std::string& characteristicPath,
                                        const std::vector<uint8_t>& data) {
    GError* error = nullptr;
    GDBusProxy* charaProxy = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        characteristicPath.c_str(),
        BLUEZ_INTERFACE_GATT_CHARACTERISTIC,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create characteristic proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    GVariantBuilder* builder = g_variant_builder_new(G_VARIANT_TYPE("ay"));
    for (uint8_t byte : data) {
        g_variant_builder_add(builder, "y", byte);
    }

    GVariant* parameters = g_variant_new("(@aya{sv})", g_variant_builder_end(builder), nullptr);
    g_variant_builder_unref(builder);

    g_dbus_proxy_call_sync(
        charaProxy,
        "WriteValue",
        parameters,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    g_object_unref(charaProxy);

    if (error) {
        std::cerr << "Failed to write characteristic: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    return true;
}

bool BleDeviceObject::EnableNotification(const std::string& servicePath, const std::string& characteristicPath,
                                       std::function<void(const std::vector<uint8_t>&)> callback) {
    GError* error = nullptr;
    GDBusProxy* charaProxy = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        characteristicPath.c_str(),
        BLUEZ_INTERFACE_GATT_CHARACTERISTIC,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create characteristic proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    g_dbus_proxy_call_sync(
        charaProxy,
        "StartNotify",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to enable notification: " << error->message << std::endl;
        g_error_free(error);
        g_object_unref(charaProxy);
        return false;
    }

    notificationCallbacks_[characteristicPath] = callback;
    g_object_unref(charaProxy);

    return true;
}

bool BleDeviceObject::DisableNotification(const std::string& servicePath, const std::string& characteristicPath) {
    GError* error = nullptr;
    GDBusProxy* charaProxy = g_dbus_proxy_new_sync(
        dbusConnection_,
        G_DBUS_PROXY_FLAGS_NONE,
        nullptr,
        BLUEZ_BUS_NAME,
        characteristicPath.c_str(),
        BLUEZ_INTERFACE_GATT_CHARACTERISTIC,
        nullptr,
        &error
    );

    if (error) {
        std::cerr << "Failed to create characteristic proxy: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    g_dbus_proxy_call_sync(
        charaProxy,
        "StopNotify",
        nullptr,
        G_DBUS_CALL_FLAGS_NONE,
        -1,
        nullptr,
        &error
    );

    g_object_unref(charaProxy);

    if (error) {
        std::cerr << "Failed to disable notification: " << error->message << std::endl;
        g_error_free(error);
        return false;
    }

    notificationCallbacks_.erase(characteristicPath);
    return true;
}
