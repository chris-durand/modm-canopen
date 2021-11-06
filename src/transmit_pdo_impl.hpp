#ifndef CANOPEN_TRANSMIT_PDO_HPP
#error "Do not include this file directly, include transmit_pdo.hpp instead!"
#endif

namespace modm_canopen
{

template<typename OD>
void TransmitPdo<OD>::setCanId(uint32_t canId)
{
    canId_ = canId;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::setActive()
{
    if(const auto error = validateMappings(); error != SdoErrorCode::NoError) {
        return error;
    }

    active_ = true;

    return SdoErrorCode::NoError;
}

template<typename OD>
void TransmitPdo<OD>::setInactive()
{
    active_ = false;
}

template<typename OD>
bool TransmitPdo<OD>::isActive() const
{
    return active_;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::setMappingCount(uint_fast8_t count)
{
    if (active_ || count > MaxMappingCount) {
        return SdoErrorCode::UnsupportedAccess;
    }

    unsigned totalSize = 0;
    for (uint_fast8_t i = 0; i < count; ++i) {
        const auto error = validateMapping(mappings_[i]);
        if (error != SdoErrorCode::NoError) {
            return error;
        }
        const auto entry = OD::map.lookup(mappings_[i].address);
        mappingTypes_[i] = entry->dataType;
        totalSize += mappings_[i].bitLength;
    }
    if (totalSize > 8*8) {
        return SdoErrorCode::MappingsExceedPdoLength;
    }

    mappingCount_ = count;
    return SdoErrorCode::NoError;
}

template<typename OD>
uint_fast8_t TransmitPdo<OD>::mappingCount() const
{
    return mappingCount_;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::setMapping(uint_fast8_t index, PdoMapping mapping)
{
    const auto error = validateMapping(mapping);
    if (error == SdoErrorCode::NoError) {
        mappings_[index] = mapping;
    }
    return error;
}

template<typename OD>
PdoMapping TransmitPdo<OD>::mapping(uint_fast8_t index) const
{
    return mappings_[index];
}

template<typename OD>
template<typename Callback>
std::optional<modm::can::Message> TransmitPdo<OD>::getMessage(Callback&& cb)
{
    sendOnEvent_.updated_ = false;
    modm::can::Message message{canId_};
    message.setExtended(false);

    if (active_ && mappingCount_ > 0) {
        std::size_t index = 0;
        for (uint_fast8_t i = 0; i < mappingCount_; ++i) {
            const auto address = mappings_[i].address;
            const auto size = mappings_[i].bitLength / 8;
            const auto value = std::forward<Callback>(cb)(address);
            const auto* ptr = std::get_if<Value>(&value);
            if (!ptr) return std::nullopt;
            valueToBytes(*ptr, message.data + index);
            index += size;
        }
        message.length = index;
    }
    return message;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::validateMappings()
{
    unsigned totalSize = 0;
    for (uint_fast8_t i = 0; i < mappingCount_; ++i) {
        const auto error = validateMapping(mappings_[i]);
        if (error != SdoErrorCode::NoError) {
            return error;
        }
        const auto entry = OD::map.lookup(mappings_[i].address);
        mappingTypes_[i] = entry->dataType;
        totalSize += mappings_[i].bitLength;
    }
    if (totalSize > 8*8) {
        return SdoErrorCode::MappingsExceedPdoLength;
    }
    return SdoErrorCode::NoError;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::validateMapping(PdoMapping mapping)
{
    const auto entry = OD::map.lookup(mapping.address);
    if (!entry) {
        return SdoErrorCode::ObjectDoesNotExist;
    }
    if (!entry->isTransmitPdoMappable()) {
        return SdoErrorCode::PdoMappingError;
    }
    if (getDataTypeSize(entry->dataType)*8 != mapping.bitLength) {
        return SdoErrorCode::PdoMappingError;
    }
    return SdoErrorCode::NoError;
}

template<typename OD>
void TransmitPdo<OD>::sync()
{
    sync_ = true;
}

template<typename OD>
void TransmitPdo<OD>::setValueUpdated()
{
    sendOnEvent_.updated_ = true;
}

template<typename OD>
template<typename Callback>
std::optional<modm::can::Message> TransmitPdo<OD>::nextMessage(Callback&& cb)
{
    const bool send = (transmitMode_ == TransmitMode::OnSync && sync_)
        || sendOnEvent_.send();

    if (send) {
        return getMessage(std::forward<Callback>(cb));
    } else {
        return std::nullopt;
    }
}

template<typename OD>
void TransmitPdo<OD>::setTransmitMode(TransmitMode mode)
{
    transmitMode_ = mode;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::setEventTimeout(uint16_t milliseconds)
{
    sendOnEvent_.eventTimeout_ = std::chrono::milliseconds(milliseconds);
    return SdoErrorCode::NoError;
}

template<typename OD>
SdoErrorCode TransmitPdo<OD>::setInhibitTime(uint16_t inhibitTime_100us)
{
    sendOnEvent_.inhibitTime_ = std::chrono::microseconds(inhibitTime_100us*100);
    return SdoErrorCode::NoError;
}

template<typename OD>
uint16_t TransmitPdo<OD>::eventTimeout() const
{
    return std::chrono::duration_cast<modm::Duration>(sendOnEvent_.eventTimeout_).count();
}

template<typename OD>
uint16_t TransmitPdo<OD>::inhibitTime() const
{
    return sendOnEvent_.inhibitTime_.count() / 100;
}

}
