#ifndef CANOPEN_CANOPEN_DEVICE_HPP
#error "Do not include this file directly, use canopen_device.hpp instead"
#endif

namespace modm_canopen
{
namespace detail
{

template<Address address>
struct missing_read_handler {
    static_assert(address == Address{},
                  "Read handler not registered for at least one object");
};

template<Address address>
struct missing_write_handler {
    static_assert(address == Address{},
                  "Write handler not registered for at least one object");
};

}

template<typename OD, typename... Protocols>
auto CanopenDevice<OD, Protocols...>::write(Address address, Value value) -> SdoErrorCode
{
    auto entry = OD::map.lookup(address);
    if (!entry) {
        return SdoErrorCode::ObjectDoesNotExist;
    }
    if (value.index() != static_cast<uint32_t>(entry->dataType)) {
        return SdoErrorCode::UnsupportedAccess;
    }

    auto handler = accessHandlers.lookupWriteHandler(address);
    if (handler) {
        const auto result = callWriteHandler(*handler, value);
        if (result == SdoErrorCode::NoError) {
            setValueChanged(address);
        }
        return result;
    } else if (!entry->isWritable()) {
        return SdoErrorCode::WriteOfReadOnlyObject;
    } else { // TODO: can this happen?
        return SdoErrorCode::UnsupportedAccess;
    }
}

template<typename OD, typename... Protocols>
auto CanopenDevice<OD, Protocols...>::write(Address address,
                                            std::span<const uint8_t> data,
                                            int8_t size) -> SdoErrorCode
{
    auto entry = OD::map.lookup(address);
    if (!entry) {
        return SdoErrorCode::ObjectDoesNotExist;
    }
    if (!entry->isWritable()) {
        return SdoErrorCode::WriteOfReadOnlyObject;
    }

    const auto objectSize = getDataTypeSize(entry->dataType);
    const bool sizeIsValid = (objectSize <= data.size()) &&
        ((size == -1) || (size == int8_t(objectSize)));
    if (!sizeIsValid) {
        return SdoErrorCode::UnsupportedAccess;
    }

    auto handler = accessHandlers.lookupWriteHandler(address);
    if (handler) {
        const Value value = valueFromBytes(entry->dataType, data.data());
        const auto result = callWriteHandler(*handler, value);
        if (result == SdoErrorCode::NoError) {
            setValueChanged(address);
        }

        return result;
    }
    return SdoErrorCode::UnsupportedAccess;
}

template<typename OD, typename... Protocols>
auto CanopenDevice<OD, Protocols...>::read(Address address) -> std::variant<Value, SdoErrorCode>
{
    auto handler = accessHandlers.lookupReadHandler(address);
    if (handler) {
        return callReadHandler(*handler);
    } else {
        auto entry = OD::map.lookup(address);
        if (!entry) {
            return SdoErrorCode::ObjectDoesNotExist;
        } else if (entry->isReadable()) {
            return SdoErrorCode::ReadOfWriteOnlyObject;
        }
    }
    return SdoErrorCode::UnsupportedAccess;
}

template<typename OD, typename... Protocols>
template<typename MessageCallback>
void CanopenDevice<OD, Protocols...>::processMessage(const modm::can::Message& message, MessageCallback&& cb)
{
    if ((message.identifier & 0x7f) == nodeId_) {
        for (auto& rpdo : receivePdos_) {
            rpdo.processMessage(message, [](Address address, Value value) {
                write(address, value);
            });
        }
        sdoServer_.processMessage(message, std::forward<MessageCallback>(cb));
    }
}

template<typename OD, typename... Protocols>
template<typename MessageCallback>
void CanopenDevice<OD, Protocols...>::update(MessageCallback&& cb)
{
    for (auto& tpdo : transmitPdos_) {
        if (tpdo.isActive()) {
            auto message = tpdo.nextMessage([](Address address) {
                return read(address);
            });
            if (message) {
                std::forward<MessageCallback>(cb)(*message);
            }
        }
    }
}

template<typename OD, typename... Protocols>
void CanopenDevice<OD, Protocols...>::setValueChanged(Address address)
{
    for (auto& tpdo : transmitPdos_) {
        if (tpdo.isActive()) {
            for (uint_fast8_t i = 0; i < tpdo.mappingCount(); ++i) {
                if (tpdo.mapping(i).address == address) {
                    tpdo.setValueUpdated();
                    break;
                }
            }
        }
    }
}

template<typename OD, typename... Protocols>
void CanopenDevice<OD, Protocols...>::setNodeId(uint8_t id)
{
    nodeId_ = id & 0x7f;
    static_assert(transmitPdos_.size() == 4);
    static_assert(receivePdos_.size() == 4);
    for (int i = 0; i < 4; ++i) {
        transmitPdos_[i].setCanId((0x100 * (i + 1) + 0x80) | nodeId_);
    }
    for (int i = 0; i < 4; ++i) {
        receivePdos_[i].setCanId(0x100 * (i + 2) | nodeId_);
    }
    sdoServer_.setNodeId(id);
}

template<typename OD, typename... Protocols>
uint8_t CanopenDevice<OD, Protocols...>::nodeId()
{
    return nodeId_;
}

template<typename OD, typename... Protocols>
constexpr auto CanopenDevice<OD, Protocols...>::registerHandlers() -> HandlerMap<OD>
{
    HandlerMap<OD> handlers;
    ReceivePdoConfigurator<CanopenDevice>{}.registerHandlers(handlers);
    TransmitPdoConfigurator<CanopenDevice>{}.registerHandlers(handlers);
    (Protocols{}.registerHandlers(handlers), ...);

    return handlers;
}

template<typename OD, typename... Protocols>
constexpr auto CanopenDevice<OD, Protocols...>::constructHandlerMap() -> HandlerMap<OD>
{
    constexpr HandlerMap<OD> handlers = registerHandlers();
    detail::missing_read_handler<findMissingReadHandler(handlers)>();
    detail::missing_write_handler<findMissingWriteHandler(handlers)>();
    return handlers;
}
}
