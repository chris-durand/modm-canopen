#ifndef CANOPEN_HANDLER_MAP_HPP
#define CANOPEN_HANDLER_MAP_HPP

#include <cstdint>
#include <variant>
#include "sdo_error.hpp"
#include "constexpr_map.hpp"

namespace modm_canopen
{

template<typename T>
using ReadFunction = T(*)();

template<typename T>
using WriteFunction = SdoErrorCode(*)(T);

using ReadHandler = std::variant<
    std::monostate,
    ReadFunction<uint8_t>,
    ReadFunction<uint16_t>,
    ReadFunction<uint32_t>,
    ReadFunction<uint64_t>,
    ReadFunction<int8_t>,
    ReadFunction<int16_t>,
    ReadFunction<int32_t>,
    ReadFunction<int64_t>
>;


using WriteHandler = std::variant<
    std::monostate,
    WriteFunction<uint8_t>,
    WriteFunction<uint16_t>,
    WriteFunction<uint32_t>,
    WriteFunction<uint64_t>,
    WriteFunction<int8_t>,
    WriteFunction<int16_t>,
    WriteFunction<int32_t>,
    WriteFunction<int64_t>
>;

template<typename OD>
class HandlerMap
{
public:
    static constexpr std::size_t ReadHandlerCount  = readableEntryCount<OD>();
    static constexpr std::size_t WriteHandlerCount = writableEntryCount<OD>();

    using ReadHandlerMap = ConstexprMap<Address, ReadHandler, ReadHandlerCount>;
    using WriteHandlerMap = ConstexprMap<Address, WriteHandler, WriteHandlerCount>;

    constexpr HandlerMap() {}

private:
    static constexpr auto makeReadHandlerMap() -> ReadHandlerMap
    {
        ConstexprMapBuilder<Address, ReadHandler, ReadHandlerCount> builder{};
        for (const auto& [address, entry] : OD::map) {
            if (entry.isReadable()) {
                builder.insert(address, ReadHandler{});
            }
        }
        return builder.buildMap();
    }

    static constexpr auto makeWriteHandlerMap() -> WriteHandlerMap
    {
        ConstexprMapBuilder<Address, WriteHandler, WriteHandlerCount> builder{};
        for (const auto& [address, entry] : OD::map) {
            if (entry.isWritable()) {
                builder.insert(address, WriteHandler{});
            }
        }
        return builder.buildMap();
    }

    ReadHandlerMap readHandlers = makeReadHandlerMap();
    WriteHandlerMap writeHandlers = makeWriteHandlerMap();

public:
    constexpr auto lookupReadHandler(Address address) const
    {
        return readHandlers.lookup(address);
    }

    constexpr auto lookupWriteHandler(Address address) const
    {
        return writeHandlers.lookup(address);
    }

    template<Address address, typename ReturnT>
    constexpr void setReadHandler(ReturnT(*func)())
    {
        ReadHandler handler = func;
        constexpr auto entry = OD::map.lookup(address);
        static_assert(entry, "Object not found");

        // if constexpr prevents ugly compiler output when assertion triggers
        if constexpr (entry) {
            constexpr bool accessValid = entry->isReadable();
            static_assert(accessValid, "Cannot register read handler for write-only object");

            constexpr auto handlerIndex = ReadHandler(static_cast<ReturnT(*)()>(nullptr)).index();
            constexpr auto odIndex = static_cast<std::size_t>(entry->dataType);
            static_assert(odIndex == handlerIndex, "Invalid read handler type for entry");

            if constexpr (accessValid && (odIndex == handlerIndex)) {
                *readHandlers.lookup(address) = handler;
            }
        }
    }

    template<Address address, typename Param>
    constexpr void setWriteHandler(SdoErrorCode(*func)(Param))
    {
        WriteHandler handler = func;
        constexpr auto entry = OD::map.lookup(address);
        static_assert(entry, "Object not found");

        // if constexpr prevents ugly compiler output when assertion triggers
        if constexpr (entry) {
            constexpr bool accessValid = entry->isWritable();
            static_assert(accessValid, "Cannot register write handler for read-only object");

            constexpr auto odIndex = static_cast<std::size_t>(entry->dataType);
            constexpr auto handlerIndex = WriteHandler(static_cast<SdoErrorCode(*)(Param)>(nullptr)).index();
            static_assert(odIndex == handlerIndex, "Invalid write handler type for entry");

            if constexpr (accessValid && (odIndex == handlerIndex)) {
                *writeHandlers.lookup(address) = handler;
            }
        }
    }
};

static_assert(ReadHandler(std::monostate{}).index() == size_t(DataType::Empty));
static_assert(ReadHandler(ReadFunction<uint8_t >{}).index() == size_t(DataType::UInt8));
static_assert(ReadHandler(ReadFunction<uint16_t>{}).index() == size_t(DataType::UInt16));
static_assert(ReadHandler(ReadFunction<uint32_t>{}).index() == size_t(DataType::UInt32));
static_assert(ReadHandler(ReadFunction<uint64_t>{}).index() == size_t(DataType::UInt64));
static_assert(ReadHandler(ReadFunction< int8_t >{}).index() == size_t(DataType::Int8));
static_assert(ReadHandler(ReadFunction< int16_t>{}).index() == size_t(DataType::Int16));
static_assert(ReadHandler(ReadFunction< int32_t>{}).index() == size_t(DataType::Int32));
static_assert(ReadHandler(ReadFunction< int64_t>{}).index() == size_t(DataType::Int64));

static_assert(WriteHandler(std::monostate{}).index() == size_t(DataType::Empty));
static_assert(WriteHandler(WriteFunction<uint8_t >{}).index() == size_t(DataType::UInt8));
static_assert(WriteHandler(WriteFunction<uint16_t>{}).index() == size_t(DataType::UInt16));
static_assert(WriteHandler(WriteFunction<uint32_t>{}).index() == size_t(DataType::UInt32));
static_assert(WriteHandler(WriteFunction<uint64_t>{}).index() == size_t(DataType::UInt64));
static_assert(WriteHandler(WriteFunction< int8_t >{}).index() == size_t(DataType::Int8));
static_assert(WriteHandler(WriteFunction< int16_t>{}).index() == size_t(DataType::Int16));
static_assert(WriteHandler(WriteFunction< int32_t>{}).index() == size_t(DataType::Int32));
static_assert(WriteHandler(WriteFunction< int64_t>{}).index() == size_t(DataType::Int64));

template<typename OD>
constexpr Address findMissingReadHandler(const HandlerMap<OD>& map)
{
    for (const auto& entry : OD::map) {
        if (entry.second.isReadable()) {
            auto lookup = map.lookupReadHandler(entry.second.address);
            if (!lookup || std::holds_alternative<std::monostate>(*lookup)) {
                return entry.second.address;
            }
        }
    }
    return Address{};
}

template<typename OD>
constexpr Address findMissingWriteHandler(const HandlerMap<OD>& map)
{
    for (const auto& entry : OD::map) {
        if (entry.second.isWritable()) {
            auto lookup = map.lookupWriteHandler(entry.second.address);
            if (!lookup || std::holds_alternative<std::monostate>(*lookup)) {
                return entry.second.address;
            }
        }
    }
    return Address{};
}

inline Value callReadHandler(ReadHandler h)
{
    static_assert(Value(std::monostate{}).index() == size_t(DataType::Empty));

    switch (DataType(h.index())) {
    case DataType::Empty:
        return Value{};
    case DataType::UInt8:
        return Value(std::get<ReadFunction<uint8_t>>(h)());
    case DataType::UInt16:
        return Value(std::get<ReadFunction<uint16_t>>(h)());
    case DataType::UInt32:
        return Value(std::get<ReadFunction<uint32_t>>(h)());
    case DataType::UInt64:
        return Value(std::get<ReadFunction<uint64_t>>(h)());
    case DataType::Int8:
        return Value(std::get<ReadFunction<int8_t>>(h)());
    case DataType::Int16:
        return Value(std::get<ReadFunction<int16_t>>(h)());
    case DataType::Int32:
        return Value(std::get<ReadFunction<int32_t>>(h)());
    case DataType::Int64:
        return Value(std::get<ReadFunction<int64_t>>(h)());
    }
    return Value{};
}

inline SdoErrorCode callWriteHandler(WriteHandler h, Value value)
{
    switch (DataType(h.index())) {
    case DataType::UInt8:
        return std::get<WriteFunction<uint8_t>>(h)(*std::get_if<uint8_t>(&value));
    case DataType::UInt16:
        return std::get<WriteFunction<uint16_t>>(h)(*std::get_if<uint16_t>(&value));
    case DataType::UInt32:
        return std::get<WriteFunction<uint32_t>>(h)(*std::get_if<uint32_t>(&value));
    case DataType::UInt64:
        return std::get<WriteFunction<uint64_t>>(h)(*std::get_if<uint64_t>(&value));
    case DataType::Int8:
        return std::get<WriteFunction<int8_t>>(h)(*std::get_if<int8_t>(&value));
    case DataType::Int16:
        return std::get<WriteFunction<int16_t>>(h)(*std::get_if<int16_t>(&value));
    case DataType::Int32:
        return std::get<WriteFunction<int32_t>>(h)(*std::get_if<int32_t>(&value));
    case DataType::Int64:
        return std::get<WriteFunction<int64_t>>(h)(*std::get_if<int64_t>(&value));
    case DataType::Empty:
        break;
    }
    return SdoErrorCode::GeneralError;
}

}

#endif // CANOPEN_HANDLER_MAP_HPP
