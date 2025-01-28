#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>

class UuidObject {
public:
    UuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4);
    ~UuidObject() = default;

    std::string GetUuidString() const;
    void GetUuid128(uint8_t* dest) const;

private:
    uint32_t data1_;
    uint32_t data2_;
    uint32_t data3_;
    uint32_t data4_;
    std::string uuidString_;
};

class UuidManager {
public:
    static UuidManager& GetInstance();
    UuidObject* GetOrCreateUuidObject(uint32_t d1, uint32_t d2, uint32_t d3, uint32_t d4);

private:
    UuidManager() = default;
    ~UuidManager() = default;
    UuidManager(const UuidManager&) = delete;
    UuidManager& operator=(const UuidManager&) = delete;

    std::map<std::string, std::unique_ptr<UuidObject>> uuidMap_;
    std::mutex mutex_;
};
