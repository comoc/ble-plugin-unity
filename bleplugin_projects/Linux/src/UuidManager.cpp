#include "UuidManager.h"
#include <sstream>
#include <iomanip>

UuidObject::UuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4)
    : data1_(d1), data2_(d2), data3_(d3), data4_(d4) {
    // UUIDの文字列表現を生成
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << data1_ << "-";
    ss << std::setw(4) << ((data2_ >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (data2_ & 0xFFFF) << "-";
    ss << std::setw(4) << ((data3_ >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (data3_ & 0xFFFF);
    ss << std::setw(8) << data4_;
    uuidString_ = ss.str();
}

std::string UuidObject::GetUuidString() const {
    return uuidString_;
}

void UuidObject::GetUuid128(uint8_t* dest) const {
    // 128ビットUUIDのバイト配列を生成
    uint32_t* dest32 = reinterpret_cast<uint32_t*>(dest);
    dest32[0] = data1_;
    dest32[1] = data2_;
    dest32[2] = data3_;
    dest32[3] = data4_;
}

UuidManager& UuidManager::GetInstance() {
    static UuidManager instance;
    return instance;
}

UuidObject* UuidManager::GetOrCreateUuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4) {
    std::lock_guard<std::mutex> lock(mutex_);

    // UUIDの文字列表現を生成
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    ss << std::setw(8) << d1 << "-";
    ss << std::setw(4) << ((d2 >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (d2 & 0xFFFF) << "-";
    ss << std::setw(4) << ((d3 >> 16) & 0xFFFF) << "-";
    ss << std::setw(4) << (d3 & 0xFFFF);
    ss << std::setw(8) << d4;
    std::string uuidStr = ss.str();

    // 既存のUUIDを検索
    auto it = uuidMap_.find(uuidStr);
    if (it != uuidMap_.end()) {
        return it->second.get();
    }

    // 新しいUUIDオブジェクトを作成
    auto newUuid = std::make_unique<UuidObject>(d1, d2, d3, d4);
    UuidObject* result = newUuid.get();
    uuidMap_[uuidStr] = std::move(newUuid);
    return result;
}
