#ifndef CANOPEN_OBJECT_DICTIONARY_HPP
#define CANOPEN_OBJECT_DICTIONARY_HPP

#include "object_dictionary_common.hpp"
#include "generated/object_dictionary.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <optional>
#include <utility>
#include <variant>

#include "sdo_error.hpp"

namespace modm_canopen
{

using Value = std::variant<std::monostate,
                            uint8_t, uint16_t, uint32_t, uint64_t,
                            int8_t,  int16_t,  int32_t,  int64_t>;


template<typename Map>
constexpr std::size_t readableEntryCount()
{
    const auto isReadable = [](const std::pair<Address, Entry>& elem) {
        return elem.second.isReadable() ? 1u : 0u;
    };
    return std::transform_reduce(Map::map.begin(), Map::map.end(), 0u,
                                 std::plus<>{},
                                 isReadable);
}

template<typename Map>
constexpr std::size_t writableEntryCount()
{
    const auto isWritable = [](const std::pair<Address, Entry>& elem) {
        return elem.second.isWritable() ? 1u : 0u;
    };
    return std::transform_reduce(Map::map.begin(), Map::map.end(), 0u,
                                 std::plus<>{},
                                 isWritable);
}

inline size_t getDataTypeSize(DataType type)
{
    switch (type) {
    case DataType::Empty:
        return 0;
    case DataType::UInt8:
        return 1;
    case DataType::UInt16:
        return 2;
    case DataType::UInt32:
        return 4;
    case DataType::UInt64:
        return 8;
    case DataType::Int8:
        return 1;
    case DataType::Int16:
        return 2;
    case DataType::Int32:
        return 4;
    case DataType::Int64:
        return 8;
    }
    return 0;
}

inline size_t getValueSize(const Value& value)
{
    return getDataTypeSize(DataType(value.index()));
}

inline bool typeSupportsExpediteTransfer(DataType type)
{
    return (type != DataType::Empty) &&
        (type != DataType::UInt64) &&
        (type != DataType::Int64);
}

inline bool valueSupportsExpediteTransfer(const Value& value)
{
    const auto type = static_cast<DataType>(value.index());
    return typeSupportsExpediteTransfer(type);
}

inline Value valueFromBytes(DataType type, const uint8_t* data)
{
    switch (type) {
    case DataType::UInt8:
        return Value(uint8_t(data[0]));
    case DataType::UInt16:
        return Value(uint16_t(data[0] | (data[1] << 8)));
    case DataType::UInt32:
        return Value(uint32_t(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)));
    case DataType::UInt64:
        return Value(uint64_t(data[0])  | (uint64_t(data[1]) << 8)
            | (uint64_t(data[2]) << 16) | (uint64_t(data[3]) << 24)
            | (uint64_t(data[4]) << 32) | (uint64_t(data[5]) << 40)
            | (uint64_t(data[6]) << 48) | (uint64_t(data[7]) << 56));
    case DataType::Int8:
        return Value(int8_t(data[0]));
    case DataType::Int16:
        return Value(int16_t(data[0] | (data[1] << 8)));
    case DataType::Int32:
        return Value(int32_t(data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)));
    case DataType::Int64:
        return Value(int64_t(data[0])  | (int64_t(data[1]) << 8)
            | (int64_t(data[2]) << 16) | (int64_t(data[3]) << 24)
            | (int64_t(data[4]) << 32) | (int64_t(data[5]) << 40)
            | (int64_t(data[6]) << 48) | (int64_t(data[7]) << 56));
    case DataType::Empty:
        return Value{};
    }
    return Value{};

}

inline void valueToBytes(Value value, uint8_t* data)
{
    // TODO: hack
    std::visit([data](auto value) {
        std::memcpy(data, &value, sizeof(value));
    }, value);
}

}

#endif // CANOPEN_OBJECT_DICTIONARY_HPP
