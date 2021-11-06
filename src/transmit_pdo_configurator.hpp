#ifndef CANOPEN_TRANSMIT_PDO_CONFIGURATOR_HPP
#define CANOPEN_TRANSMIT_PDO_CONFIGURATOR_HPP

#include <cstdint>
#include "object_dictionary_common.hpp"

namespace modm_canopen
{

template<typename Device>
class TransmitPdoConfigurator
{
public:
    template<uint8_t pdo>
    constexpr void registerPdoConfigObjects(Device::Map& map)
    {
        auto& tpdo = Device::transmitPdos_[pdo];
        // highest sub-index supported
        map.template setReadHandler<Address{0x1800 + pdo, 0}>(
            +[]() -> uint8_t { return 5; });

        // RPDO COB-ID
        map.template setReadHandler<Address{0x1800 + pdo, 1}>(
            +[]() -> uint32_t { return tpdo.cobId(); });

        map.template setWriteHandler<Address{0x1800 + pdo, 1}>(
            +[](uint32_t cobId) { return setTransmitPdoCobId(pdo, cobId); });

        map.template setReadHandler<Address{0x1800 + pdo, 2}>(
            // 0xFF: async
            +[]() -> uint8_t { return 0xFF; });

        map.template setWriteHandler<Address{0x1800 + pdo, 2}>(
            // 0xFF: async
            +[](uint8_t transmitMode) -> SdoErrorCode { return transmitMode == 0xFF ? SdoErrorCode::NoError : SdoErrorCode::UnsupportedAccess; });

        map.template setReadHandler<Address{0x1800 + pdo, 3}>(
            +[]() -> uint16_t { return tpdo.inhibitTime(); });

        map.template setWriteHandler<Address{0x1800 + pdo, 3}>(
            +[](uint16_t inhibitTime) { return tpdo.setInhibitTime(inhibitTime); });

        map.template setReadHandler<Address{0x1800 + pdo, 5}>(
            +[]() -> uint16_t { return tpdo.eventTimeout(); });

        map.template setWriteHandler<Address{0x1800 + pdo, 5}>(
            +[](uint16_t timeout_ms) { return tpdo.setEventTimeout(timeout_ms); });
    }

    template<uint8_t pdo, uint8_t mappingIndex>
    constexpr void registerMappingObjects(Device::Map& map)
    {
        auto& tpdo = Device::transmitPdos_[pdo];
        map.template setReadHandler<Address{0x1A00 + pdo, mappingIndex + 1}>(
            +[]() -> uint32_t { return tpdo.mapping(mappingIndex).encode(); });

        map.template setWriteHandler<Address{0x1A00 + pdo, mappingIndex + 1}>(
            +[](uint32_t mapping) { return tpdo.setMapping(mappingIndex, PdoMapping::decode(mapping)); });
    }

    template<uint8_t pdo>
    constexpr void registerMappingConfigObjects(Device::Map& map)
    {
        auto& tpdos = Device::transmitPdos_;
        // mapping count
        map.template setReadHandler<Address{0x1A00 + pdo, 0}>(
            +[]() -> uint8_t { return tpdos[pdo].mappingCount(); });

        map.template setWriteHandler<Address{0x1A00 + pdo, 0}>(
            +[](uint8_t count) { return tpdos[pdo].setMappingCount(count); });
        registerMappingObjects<pdo, 0>(map);
        registerMappingObjects<pdo, 1>(map);
        registerMappingObjects<pdo, 2>(map);
        registerMappingObjects<pdo, 3>(map);
        registerMappingObjects<pdo, 4>(map);
        registerMappingObjects<pdo, 5>(map);
        registerMappingObjects<pdo, 6>(map);
        registerMappingObjects<pdo, 7>(map);
    }

    constexpr void registerHandlers(Device::Map& map)
    {
        registerPdoConfigObjects<0>(map);
        registerPdoConfigObjects<1>(map);
        registerPdoConfigObjects<2>(map);
        registerPdoConfigObjects<3>(map);
        registerMappingConfigObjects<0>(map);
        registerMappingConfigObjects<1>(map);
        registerMappingConfigObjects<2>(map);
        registerMappingConfigObjects<3>(map);
    }

private:
    static SdoErrorCode setTransmitPdoCobId(uint_fast8_t index, uint32_t cobId)
    {
        auto& tpdo = Device::transmitPdos_[index];
        const uint32_t canId = 0x80 + 0x100 * (1 + index) + Device::nodeId();
        const uint32_t canIdMask = (1u << 30) - 1;
        // changing can id is not supported
        if ((cobId & canIdMask) != canId) {
            return SdoErrorCode::InvalidValue;
        }
        const bool enabled = !(cobId & (1u << 31));
        if (enabled) {
            return tpdo.setActive();
        } else {
            tpdo.setInactive();
            return SdoErrorCode::NoError;
        }
    }
};

}

#endif // CANOPEN_TRANSMIT_PDO_CONFIGURATOR_HPP
