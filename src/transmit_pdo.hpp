#ifndef CANOPEN_TRANSMIT_PDO_HPP
#define CANOPEN_TRANSMIT_PDO_HPP

#include "pdo_common.hpp"
#include <array>
#include <optional>
#include <variant>
#include <modm/architecture/interface/clock.hpp>
#include <modm/architecture/interface/can_message.hpp>
#include <modm/processing/timer/timestamp.hpp>

namespace modm_canopen
{

struct SendOnEvent
{
    modm::PreciseDuration eventTimeout_{};
    modm::PreciseDuration inhibitTime_{};
    modm::PreciseTimestamp lastMessage_{};
    bool updated_ = false;

    void setValueUpdated()
    {
        updated_ = true;
    }

    bool send()
    {
        const auto now = modm::chrono::micro_clock::now();
        if (now > lastMessage_ + inhibitTime_) {
            const bool timerEnabled = (eventTimeout_.count() != 0);
            const bool timerExpired = now > (lastMessage_ + eventTimeout_);
            if (updated_ || (timerEnabled && timerExpired)) {
                lastMessage_ = now;
                return true;
            }
        }
        return false;
    }
};

enum class TransmitMode
{
    OnSync,
    OnEvent
};

template<typename OD>
class TransmitPdo;

template<typename OD>
modm::can::Message createPdoMessage(const TransmitPdo<OD>& pdo, uint16_t canId);

// TODO: de-duplicate code with ReceivePdo
template<typename OD>
class TransmitPdo
{
private:
    static constexpr std::size_t MaxMappingCount{8};

public:
    void setCanId(uint32_t canId);

    SdoErrorCode setActive();
    void setInactive();
    bool isActive() const;

    SdoErrorCode setMappingCount(uint_fast8_t count);
    uint_fast8_t mappingCount() const;

    SdoErrorCode setMapping(uint_fast8_t index, PdoMapping mapping);
    PdoMapping mapping(uint_fast8_t index) const;

    void sync();
    void setValueUpdated();

    template<typename Callback>
    std::optional<modm::can::Message> nextMessage(Callback&& cb);

    void setTransmitMode(TransmitMode mode);
    // TODO: change parameter types
    SdoErrorCode setEventTimeout(uint16_t milliseconds); // 0 to disable
    SdoErrorCode setInhibitTime(uint16_t inhibitTime_100us);
    uint16_t eventTimeout() const;
    uint16_t inhibitTime() const;

    uint32_t cobId() const { return active_ ? canId_ : (canId_ | 0x8000'0000); }
    uint32_t canId() const { return canId_; }
private:
    bool active_{false};
    uint32_t canId_{};
    uint_fast8_t mappingCount_{};
    std::array<PdoMapping, MaxMappingCount> mappings_{};
    std::array<DataType, MaxMappingCount> mappingTypes_{};
    TransmitMode transmitMode_{};
    SendOnEvent sendOnEvent_{};
    bool sync_{false};

    SdoErrorCode validateMapping(PdoMapping mapping);
    SdoErrorCode validateMappings();

    template<typename Callback>
    std::optional<modm::can::Message> getMessage(Callback&& cb);
};

}

#include "transmit_pdo_impl.hpp"

#endif // CANOPEN_TRANSMIT_PDO_HPP
