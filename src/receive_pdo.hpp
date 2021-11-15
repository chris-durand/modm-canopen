#ifndef CANOPEN_RECEIVE_PDO_HPP
#define CANOPEN_RECEIVE_PDO_HPP

#include "pdo_common.hpp"
#include <array>
#include <modm/architecture/interface/can_message.hpp>

namespace modm_canopen
{

enum class ReceiveMode
{
    Async,
    // TODO: OnSync
};

// TODO: de-duplicate code with TransmitPdo
template<typename OD>
class ReceivePdo
{
private:
    static constexpr std::size_t MaxMappingCount{8};

    bool active_{false};
    uint32_t canId_{};
    uint_fast8_t mappingCount_{};
    std::array<PdoMapping, MaxMappingCount> mappings_{};
    std::array<DataType, MaxMappingCount> mappingTypes_{};

public:
    void setCanId(uint32_t canId);

    SdoErrorCode setActive();
    void setInactive();

    SdoErrorCode setMappingCount(uint_fast8_t count);
    uint_fast8_t mappingCount() const;

    SdoErrorCode setMapping(uint_fast8_t index, PdoMapping mapping);
    PdoMapping mapping(uint_fast8_t index) const;

    template<typename Callback>
    void processMessage(const modm::can::Message& message, Callback&& cb);

    uint32_t cobId() const { return active_ ? canId_ : (canId_ | 0x8000'0000); }
    uint32_t canId() const { return canId_; }
private:
    SdoErrorCode validateMapping(PdoMapping mapping);
    SdoErrorCode validateMappings();
};

}

#include "receive_pdo_impl.hpp"

#endif // CANOPEN_RECEIVE_PDO_HPP
