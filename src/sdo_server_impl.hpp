#ifndef CANOPEN_SDO_SERVER_HPP
#error "Do not include this file directly, include sdo_server.hpp instead!"
#endif

namespace modm_canopen
{

template<typename Device>
template<typename C>
void SdoServer<Device>::processMessage(const modm::can::Message& request,
                                       C&& cb)
{
    constexpr uint8_t commandUpload           = 0b010'0'00'0'0;
    constexpr uint8_t commandUploadMask       = 0b111'0'00'0'0;
    constexpr uint8_t commandExpediteDownload = 0b001'0'00'1'0;
    constexpr uint8_t commandDownloadMask     = 0b111'0'00'1'0;
    constexpr uint8_t sizeIndicated           = 0b000'0'00'0'1;

    const uint16_t canId = 0x600 | nodeId_;

    if (request.identifier == canId && request.getLength() == 8) {
        const Address address {
            .index = uint16_t((request.data[2] << 8) | request.data[1]),
            .subindex = request.data[3]
        };
        // TODO: clean-up, refactor into functions
        if ((request.data[0] & commandUploadMask) == commandUpload) {
            auto result = Device::read(address);
            if (const SdoErrorCode* error = std::get_if<SdoErrorCode>(&result); error) {
                std::forward<C>(cb)(detail::transferAbort(nodeId_, address, *error));
            } else {
                // std::get_if can't return nullptr
                // there are only two possible types: SdoErrorCode and Value
                const Value& value = *std::get_if<Value>(&result);
                if (valueSupportsExpediteTransfer(value)) {
                    std::forward<C>(cb)(detail::uploadResponse(nodeId_, address, value));
                } else {
                    std::forward<C>(cb)(detail::transferAbort(nodeId_, address, SdoErrorCode::UnsupportedAccess));
                }
            }
        } else if ((request.data[0] & commandDownloadMask) == commandExpediteDownload) {
            const uint8_t type = request.data[0];
            const int_fast8_t size = (type & sizeIndicated) ? (4-((type & 0b1100) >> 2)) : -1;
            const SdoErrorCode error = Device::write(address, std::span<const uint8_t>{&request.data[4], 4}, size);
            if (error == SdoErrorCode::NoError) {
                std::forward<C>(cb)(detail::downloadResponse(nodeId_, address));
            } else {
                std::forward<C>(cb)(detail::transferAbort(nodeId_, address, error));
            }
        } else {
            std::forward<C>(cb)(detail::transferAbort(nodeId_, address, SdoErrorCode::UnsupportedAccess));
        }
    }
}

template<typename Device>
uint8_t SdoServer<Device>::nodeId() const
{
    return nodeId_;
}

template<typename Device>
void SdoServer<Device>::setNodeId(uint8_t id)
{
    nodeId_ = id;
}

auto detail::uploadResponse(uint8_t nodeId, Address address, const Value& value)
    -> modm::can::Message
{
    modm::can::Message message{uint32_t(0x580 | nodeId), 8};
    message.setExtended(false);
    const auto sizeFlags = (0b11 & (4 - getValueSize(value))) << 2;
    message.data[0] = 0b010'0'00'1'1 | sizeFlags;
    message.data[1] = address.index & 0xFF;
    message.data[2] = (address.index & 0xFF'00) >> 8;
    message.data[3] = address.subindex;
    valueToBytes(value, &message.data[4]);
    return message;
}

auto detail::downloadResponse(uint8_t nodeId, Address address)
    -> modm::can::Message
{
    modm::can::Message message{uint32_t(0x580 | nodeId), 8};
    message.setExtended(false);
    message.data[0] = 0b011'00000;
    message.data[1] = address.index & 0xFF;
    message.data[2] = (address.index & 0xFF'00) >> 8;
    message.data[3] = address.subindex;
    return message;
}

auto detail::transferAbort(uint8_t nodeId, Address address, SdoErrorCode error)
    -> modm::can::Message
{
    modm::can::Message message{uint32_t(0x580 | nodeId), 8};
    message.setExtended(false);
    message.data[0] = 0b100'00000;
    message.data[1] = address.index & 0xFF;
    message.data[2] = (address.index & 0xFF'00) >> 8;
    message.data[3] = address.subindex;
    static_assert(sizeof(SdoErrorCode) == 4);
    memcpy(&message.data[4], &error, sizeof(SdoErrorCode));
    return message;
}

}
