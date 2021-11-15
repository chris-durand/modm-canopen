#ifndef CANOPEN_RECEIVE_PDO_CONFIGURATOR_HPP
#define CANOPEN_RECEIVE_PDO_CONFIGURATOR_HPP

#include <cstdint>
#include "object_dictionary_common.hpp"

namespace modm_canopen
{

template<typename Device>
class ReceivePdoConfigurator
{
public:
    template<uint8_t pdo>
    constexpr void registerPdoConfigObjects(Device::Map& map)
    {
        auto& rpdo = Device::receivePdos_[pdo];
        // highest sub-index supported
        map.template setReadHandler<Address{0x1400 + pdo, 0}>(
            +[]() -> uint8_t { return 2; });

        // RPDO COB-ID
        map.template setReadHandler<Address{0x1400 + pdo, 1}>(
            +[]() -> uint32_t { return rpdo.cobId(); });

        map.template setWriteHandler<Address{0x1400 + pdo, 1}>(
            +[](uint32_t cobId) { return setReceivePdoCobId(pdo, cobId); });

        // Transmission type, only async supported for now
        map.template setReadHandler<Address{0x1400 + pdo, 2}>(
            // 0xFF: async
            +[]() -> uint8_t { return 0xFF; });
    }

    template<uint8_t pdo, uint8_t mappingIndex>
    constexpr void registerMappingObjects(Device::Map& map)
    {
        auto& rpdo = Device::receivePdos_[pdo];
        map.template setReadHandler<Address{0x1600 + pdo, mappingIndex + 1}>(
            +[]() -> uint32_t { return rpdo.mapping(mappingIndex).encode(); });

        map.template setWriteHandler<Address{0x1600 + pdo, mappingIndex + 1}>(
            +[](uint32_t mapping) { return rpdo.setMapping(mappingIndex, PdoMapping::decode(mapping)); });
    }

    template<uint8_t pdo>
    constexpr void registerMappingConfigObjects(Device::Map& map)
    {
        auto& rpdos = Device::receivePdos_;
        // mapping count
        map.template setReadHandler<Address{0x1600 + pdo, 0}>(
            +[]() -> uint8_t { return rpdos[pdo].mappingCount(); });

        map.template setWriteHandler<Address{0x1600 + pdo, 0}>(
            +[](uint8_t count) { return rpdos[pdo].setMappingCount(count); });
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
    static SdoErrorCode setReceivePdoCobId(uint_fast8_t index, uint32_t cobId)
    {
        auto& rpdo = Device::receivePdos_[index];
        const uint32_t canId = 0x100 * (2 + index) + Device::nodeId();
        const uint32_t canIdMask = (1u << 30) - 1;
        // changing can id is not supported
        if ((cobId & canIdMask) != canId) {
            return SdoErrorCode::InvalidValue;
        }
        const bool enabled = !(cobId & (1u << 31));
        if (enabled) {
            return rpdo.setActive();
        } else {
            rpdo.setInactive();
            return SdoErrorCode::NoError;
        }
    }
};

}

#endif // CANOPEN_RECEIVE_PDO_CONFIGURATOR_HPP
