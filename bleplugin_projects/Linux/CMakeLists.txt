cmake_minimum_required(VERSION 3.10)
project(BlePluginLinux)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BlueZの依存関係
find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
pkg_check_modules(BLUEZ REQUIRED bluez)

# ソースファイル
set(SOURCES
    src/UnityInterface.cpp
    src/BleDeviceManager.cpp
    src/BleDeviceObject.cpp
    src/BleDeviceWatcher.cpp
    src/BluetoothAdapterChecker.cpp
    src/UuidManager.cpp
)

# ライブラリの作成
add_library(bleplugin SHARED ${SOURCES})

# インクルードディレクトリの設定
target_include_directories(bleplugin PRIVATE
    ${DBUS_INCLUDE_DIRS}
    ${BLUEZ_INCLUDE_DIRS}
    /usr/include/glib-2.0
    /usr/lib/x86_64-linux-gnu/glib-2.0/include
    src
)

# リンクライブラリの設定
target_link_libraries(bleplugin PRIVATE
    ${DBUS_LIBRARIES}
    ${BLUEZ_LIBRARIES}
)

# インストール設定
install(TARGETS bleplugin
    LIBRARY DESTINATION lib
)
