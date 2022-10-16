#ifndef CANOPEN_SDO_SERVER_HPP
#define CANOPEN_SDO_SERVER_HPP

#include <modm/architecture/interface/can_message.hpp>
#include "object_dictionary.hpp"

namespace modm_canopen
{

template<typename Device>
class SdoServer
{
public:
    using ObjectDictionary = Device::ObjectDictionary;

    uint8_t nodeId() const;
    void setNodeId(uint8_t id);

    template<typename MessageCallback>
    static void processMessage(const modm::can::Message& request,
                               MessageCallback&& responseCallback);

private:
    static inline uint8_t nodeId_{};
};

namespace detail
{
    inline auto uploadResponse(uint8_t nodeId, Address address, const Value& value)
        -> modm::can::Message;

    inline auto downloadResponse(uint8_t nodeId, Address address)
        -> modm::can::Message;

    inline auto transferAbort(uint8_t nodeId, Address address, SdoErrorCode error)
        -> modm::can::Message;
};

}

#include "sdo_server_impl.hpp"

#endif // CANOPEN_CANOPEN_DEVICE
