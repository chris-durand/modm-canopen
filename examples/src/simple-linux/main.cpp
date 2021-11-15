#include <modm-canopen/canopen_device.hpp>
#include <modm/platform/can/socketcan.hpp>

#include <iostream>
#include <thread>
#include <modm/debug/logger.hpp>

uint32_t value2002 = 42;

using modm_canopen::Address;
using modm_canopen::CanopenDevice;
using modm_canopen::SdoErrorCode;
using modm_canopen::generated::DefaultObjects;

struct Test
{
    template<typename ObjectDictionary>
    constexpr void registerHandlers(modm_canopen::HandlerMap<ObjectDictionary>& map)
    {
        map.template setReadHandler<Address{0x2001, 0}>(
            +[](){ return uint8_t(10); });

        map.template setReadHandler<Address{0x2002, 0}>(
            +[](){ return value2002; });

        map.template setWriteHandler<Address{0x2002, 0}>(
            +[](uint32_t value)
            {
                MODM_LOG_INFO << "setting 0x2002,0 to " << value << modm::endl;
                value2002 = value;
                return SdoErrorCode::NoError;
            });
    }
};

int main()
{
    using Device = CanopenDevice<DefaultObjects, Test>;
    const uint8_t nodeId = 5;
    Device::initialize(nodeId);

    modm::platform::SocketCan can;
    const bool success = can.open("vcan0");
    if (!success) {
        MODM_LOG_ERROR << "Opening device vcan0 failed" << modm::endl;
    }

    auto sendMessage = [&can](const modm::can::Message& message) {
        can.sendMessage(message);
    };

    /*
    // manually setup default TPDO1 mapping
    // TODO: proper public APi
    Device::transmitPdos_[0].setInactive();
    Device::transmitPdos_[0].setMapping(0, PdoMapping{Address{0x2002, 0}, 32});
    Device::transmitPdos_[0].setMappingCount(1);
    Device::transmitPdos_[0].setActive();
    Device::transmitPdos_[0].setEventTimeout(500);
    */

    // call setValueChanged() when a TPDO mappable value changed
    // to trigger asynchronous PDO transmissions
    Device::setValueChanged(Address{0x2002, 0});

    while (true) {
        if (can.isMessageAvailable()) {
            modm::can::Message message{};
            can.getMessage(message);
            Device::processMessage(message, sendMessage);
        }
        Device::update(sendMessage);
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
}
