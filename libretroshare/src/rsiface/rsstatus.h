#include <string.h>
#include <inttypes.h>

const uint32_t RS_STATUS_OFFLINE = 0x0001;
const uint32_t RS_STATUS_AWAY    = 0x0002;
const uint32_t RS_STATUS_BUSY    = 0x0003;
const uint32_t RS_STATUS_ONLINE  = 0x0004;

class StatusInfo
{
	std::string id;
	uint32_t status;
};

class RsStatusInterface
{
	public:
virtual bool getStatus(std::string id, StatusInfo& statusInfo) = 0;
virtual bool setStatus(StatusInfo& statusInfo)                 = 0;

};
