#ifndef CANOPEN_SDO_ERROR_HPP
#define CANOPEN_SDO_ERROR_HPP

#include <cstdint>

namespace modm_canopen
{

enum class SdoErrorCode : uint32_t
{
    NoError = 0,
    UnsupportedAccess = 0x0601'0000,
    ReadOfWriteOnlyObject = 0x0601'0001,
    WriteOfReadOnlyObject = 0x0601'0002,
    ObjectDoesNotExist = 0x0602'0000,
    PdoMappingError = 0x0604'0041,
    MappingsExceedPdoLength = 0x0604'0042,
    InvalidValue = 0x0609'0030,
    GeneralError = 0x0800'0000
    // TODO: add error codes
};

}

#endif
