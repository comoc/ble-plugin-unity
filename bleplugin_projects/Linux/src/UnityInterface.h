#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* UuidHandle;
typedef void* DeviceHandle;
typedef void* WriteRequestHandle;
typedef void* ReadRequestHandle;

// BLEアダプタ管理
void _BlePluginBleAdapterStatusRequest();
int _BlePluginBleAdapterUpdate();

// プラグインの終了処理
void _BlePluginFinalize();

// UUID管理
UuidHandle _BlePluginGetOrCreateUuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4);
void _BlePluginConvertUuidUint128(UuidHandle ptr, void* out);

// スキャン管理
void _BlePluginUpdateWatcher();
void _BlePluginUpdateDevicdeManger();
void _BlePluginAddScanServiceUuid(UuidHandle uuid);
void _BlePluginStartScan();
void _BlePluginStopScan();
void _BlePluginClearScanFilter();

// スキャンデータ取得
int _BlePluginScanGetDeviceLength();
uint64_t _BlePluginScanGetDeviceAddr(int idx);
const char* _BlePluginScanGetDeviceName(int idx);
int32_t _BlePluginScanGetDeviceRssi(int idx);

// 接続・切断
DeviceHandle _BlePluginConnectDevice(uint64_t addr);
void _BlePluginDisconnectDevice(uint64_t addr);
void _BlePluginDisconnectAllDevice();
bool _BlePluginIsDeviceConnectedByAddr(uint64_t addr);
bool _BlePluginIsDeviceConnected(DeviceHandle devicePtr);
uint64_t _BlePluginDeviceGetAddr(DeviceHandle devicePtr);

// デバイス情報取得
int _BlePluginGetConectDeviceNum();
uint64_t _BlePluginGetConectDevicAddr(int idx);
DeviceHandle _BlePluginGetConnectDevicePtr(int idx);
DeviceHandle _BlePluginGetDevicePtrByAddr(uint64_t addr);
int _BlePluginDeviceCharastricsNum(DeviceHandle devicePtr);
UuidHandle _BlePluginDeviceCharastricUuid(DeviceHandle devicePtr, int idx);
UuidHandle _BlePluginDeviceCharastricServiceUuid(DeviceHandle devicePtr, int idx);

// 読み書きリクエスト
ReadRequestHandle _BlePluginReadCharacteristicRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid);
WriteRequestHandle _BlePluginWriteCharacteristicRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid, void* data, int size);

bool _BlePluginIsReadRequestComplete(ReadRequestHandle ptr);
bool _BlePluginIsReadRequestError(ReadRequestHandle ptr);
int _BlePluginCopyReadRequestData(ReadRequestHandle ptr, void* data, int maxSize);
void _BlePluginReleaseReadRequest(uint64_t deviceaddr, ReadRequestHandle ptr);

bool _BlePluginIsWriteRequestComplete(WriteRequestHandle ptr);
bool _BlePluginIsWriteRequestError(WriteRequestHandle ptr);
void _BlePluginReleaseWriteRequest(uint64_t deviceaddr, WriteRequestHandle ptr);

// 通知設定
void _BlePluginSetNotificateRequest(uint64_t addr, UuidHandle serviceUuid, UuidHandle charaUuid, bool enable);
int _BlePluginGetDeviceNotificateNum(uint64_t addr);
int _BlePluginCopyDeviceNotificateData(uint64_t addr, int idx, void* ptr, int maxSize);
UuidHandle _BlePluginGetDeviceNotificateServiceUuid(uint64_t addr, int idx);
UuidHandle _BlePluginGetDeviceNotificateCharastricsUuid(uint64_t addr, int idx);

#ifdef __cplusplus
}
#endif
