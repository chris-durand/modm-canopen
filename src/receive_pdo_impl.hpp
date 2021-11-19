#ifndef CANOPEN_RECEIVE_PDO_HPP
#error "Do not include this file directly, include receive_pdo.hpp instead!"
#endif

namespace modm_canopen
{

template<typename OD>
void ReceivePdo<OD>::setCanId(uint32_t canId)
{
    canId_ = canId;
}

template<typename OD>
SdoErrorCode ReceivePdo<OD>::setActive()
{
    if(const auto error = validateMappings(); error != SdoErrorCode::NoError) {
        return error;
    }

    active_ = true;
    return SdoErrorCode::NoError;
}

template<typename OD>
void ReceivePdo<OD>::setInactive()
{
    active_ = false;
}

template<typename OD>
SdoErrorCode ReceivePdo<OD>::setMappingCount(uint_fast8_t count)
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
uint_fast8_t ReceivePdo<OD>::mappingCount() const
{
    return mappingCount_;
}

template<typename OD>
SdoErrorCode ReceivePdo<OD>::setMapping(uint_fast8_t index, PdoMapping mapping)
{
    const auto error = validateMapping(mapping);
    if (error == SdoErrorCode::NoError) {
        mappings_[index] = mapping;
    }
    return error;
}

template<typename OD>
PdoMapping ReceivePdo<OD>::mapping(uint_fast8_t index) const
{
    return mappings_[index];
}

template<typename OD>
template<typename Callback>
void ReceivePdo<OD>::processMessage(const modm::can::Message& message, Callback&& cb)
{
    if (message.identifier != canId_) {
        return;
    }
    if (active_ && mappingCount_ > 0) {
        std::size_t totalDataSize = 0;
        for (uint_fast8_t i = 0; i < mappingCount_; ++i) {
            totalDataSize += mappings_[i].bitLength / 8;
        }
        if(totalDataSize > message.length) {
            return;
        }
        std::size_t index = 0;
        for (uint_fast8_t i = 0; i < mappingCount_; ++i) {
            const auto address = mappings_[i].address;
            const auto size = mappings_[i].bitLength / 8;
            const auto value = valueFromBytes(mappingTypes_[i], message.data + index);
            std::forward<Callback>(cb)(address, value);
            index += size;
        }
    }
}

template<typename OD>
SdoErrorCode ReceivePdo<OD>::validateMappings()
{
    return SdoErrorCode::NoError;
}

template<typename OD>
SdoErrorCode ReceivePdo<OD>::validateMapping(PdoMapping mapping)
{
    const auto entry = OD::map.lookup(mapping.address);
    if (!entry) {
        return SdoErrorCode::ObjectDoesNotExist;
    }
    if (!entry->isReceivePdoMappable()) {
        return SdoErrorCode::PdoMappingError;
    }
    if (getDataTypeSize(entry->dataType)*8 != mapping.bitLength) {
        return SdoErrorCode::PdoMappingError;
    }
    return SdoErrorCode::NoError;
}

}
