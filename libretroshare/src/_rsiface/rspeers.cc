#include "_rsiface/rspeers.h"
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef RS_USE_PGPSSL
#include <gpgme.h>
#endif

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
#include "pqi/authxpgp.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
#include "pqi/authssl.h"
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


std::ostream &operator<<(std::ostream &out, const RsPeerDetails &detail)
{
    out << "RsPeerDetail: " << detail.name << "  <" << detail.id << ">";
    out << std::endl;

    out << " email:   " << detail.email;
    out << " location:" << detail.location;
    out << " org:     " << detail.org;
    out << std::endl;

    out << " fpr:     " << detail.fpr;
    out << " authcode:" << detail.authcode;
    out << std::endl;

    out << " signers:";
    out << std::endl;

    std::list<std::string>::const_iterator it;
    for (it = detail.signers.begin();
            it != detail.signers.end(); it++)
    {
        out << "\t" << *it;
        out << std::endl;
    }
    out << std::endl;

    out << " trustLvl:    " << detail.trustLvl;
    out << " ownSign:     " << detail.ownsign;
    out << " trusted:     " << detail.trusted;
    out << std::endl;

    out << " state:       " << detail.state;
    out << " netMode:     " << detail.netMode;
    out << std::endl;

    out << " localAddr:   " << detail.localAddr;
    out << ":" << detail.localPort;
    out << std::endl;
    out << " extAddr:   " << detail.extAddr;
    out << ":" << detail.extPort;
    out << std::endl;


    out << " lastConnect:       " << detail.lastConnect;
    out << " connectPeriod:     " << detail.connectPeriod;
    out << std::endl;

    return out;
}


std::string RsPeerTrustString(uint32_t trustLvl)
{

    std::string str;

#ifdef RS_USE_PGPSSL
    switch (trustLvl)
    {
    default:
    case GPGME_VALIDITY_UNKNOWN:
        str = "GPGME_VALIDITY_UNKNOWN";
        break;
    case GPGME_VALIDITY_UNDEFINED:
        str = "GPGME_VALIDITY_UNDEFINED";
        break;
    case GPGME_VALIDITY_NEVER:
        str = "GPGME_VALIDITY_NEVER";
        break;
    case GPGME_VALIDITY_MARGINAL:
        str = "GPGME_VALIDITY_MARGINAL";
        break;
    case GPGME_VALIDITY_FULL:
        str = "GPGME_VALIDITY_FULL";
        break;
    case GPGME_VALIDITY_ULTIMATE:
        str = "GPGME_VALIDITY_ULTIMATE";
        break;
    }
    return str;
#endif

    if (trustLvl == RS_TRUST_LVL_GOOD)
    {
        str = "Good";
    }
    else if (trustLvl == RS_TRUST_LVL_MARGINAL)
    {
        str = "Marginal";
    }
    else
    {
        str = "No Trust";
    }
    return str;
}



std::string RsPeerStateString(uint32_t state)
{
    std::string str;
    if (state & RS_PEER_STATE_CONNECTED)
    {
        str = "Connected";
    }
    else if (state & RS_PEER_STATE_UNREACHABLE)
    {
        str = "Unreachable";
    }
    else if (state & RS_PEER_STATE_ONLINE)
    {
        str = "Available";
    }
    else if (state & RS_PEER_STATE_FRIEND)
    {
        str = "Offline";
    }
    else
    {
        str = "Neighbour";
    }
    return str;
}

std::string RsPeerNetModeString(uint32_t netModel)
{
    std::string str;
    if (netModel == RS_NETMODE_EXT)
    {
        str = "External Port";
    }
    else if (netModel == RS_NETMODE_UPNP)
    {
        str = "Ext (UPnP)";
    }
    else if (netModel == RS_NETMODE_UDP)
    {
        str = "UDP Mode";
    }
    else if (netModel == RS_NETMODE_UNREACHABLE)
    {
        str = "UDP Mode (Unreachable)";
    }
    else
    {
        str = "Unknown NetMode";
    }
    return str;
}


std::string RsPeerLastConnectString(uint32_t lastConnect)
{
    std::ostringstream out;
    out << lastConnect << " secs ago";
    return out.str();
}




