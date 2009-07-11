#include "rsstatus.h"

std::string RsStatusString(uint32_t status)
{
    std::string str;
    if (status == RS_STATUS_OFFLINE)
    {
        str = "Offline";
    }
    else if (status == RS_STATUS_AWAY)
    {
        str = "Away";
    }
    else if (status == RS_STATUS_BUSY)
    {
        str = "Busy";
    }
    else if (status == RS_STATUS_ONLINE)
    {
        str = "Online";
    }
    return str;
}


std::ostream& operator<<(std::ostream& out, const StatusInfo& si)
{
    out << "StatusInfo: " << std::endl;
    out << "id: " << si.id << std::endl;
    out << "status: " << si.status;
    out << " (" << RsStatusString(si.status) << ")" << std::endl;
    return out;
}
