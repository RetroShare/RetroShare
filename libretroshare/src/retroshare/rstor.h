#pragma once

// This code stands as an interface for the automatic configuration
// of Tor hidden services to be used by retroshare.
//
// The correct way to use it is to:
//
// 1 - properly set data and hidden service directories. This allowd the TorManager
//     to save its keys for the hidden service, or to load one that has previously been created
//
// 2 - call setupHiddenService(). This creates/loads the hidden service.
//
// 3 - call RsTor::start()
//
// 4 - loop/wait until  RsTor::getHiddenServiceStatus(service_id)
//     returns RsTorHiddenServiceStatus::ONLINE
//
// 5 - call RsTor::getHiddenserviceInfo to properly setup RS internal ports and addresses:
//
//         RsTor::getHiddenServiceInfo(service_id,onion_address,service_port,service_target_address,service_target_port);
//         RsTor::getProxyServerInfo(proxy_server_address,proxy_server_port) ;
//
//         rsPeers->setLocalAddress(rsPeers->getOwnId(), service_target_address, service_target_port);
//         rsPeers->setHiddenNode(rsPeers->getOwnId(), onion_address, service_port);
//         rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, proxy_server_address,proxy_server_port) ;

#include "retroshare/rsevents.h"

namespace Tor {
        class TorManager;
}

enum class RsTorManagerEventCode: uint8_t
{
    UNKNOWN                   = 0x00,
    TOR_STATUS_CHANGED        = 0x01,
    BOOTSTRAP_STATUS_CHANGED  = 0x02,
    TOR_CONNECTIVITY_CHANGED  = 0x03,
};

// Status of the Tor hidden service setup/loaded by RS

enum class RsTorHiddenServiceStatus: uint8_t {
    ERROR             = 0x00,
    NOT_CREATED       = 0x01,
    OFFLINE           = 0x02,
    ONLINE            = 0x03
};

// Status of the connection/authentication between RS and the Tor service

enum class RsTorConnectivityStatus: uint8_t {
    ERROR             = 0x00,
    NOT_CONNECTED     = 0x01,
    CONNECTING        = 0x02,
    AUTHENTICATING    = 0x03,
    CONNECTED         = 0x04
};

// Status of the Tor service with which RS is talking.

enum class RsTorStatus: uint8_t {
    UNKNOWN      = 0x00,
    OFFLINE      = 0x01,
    READY        = 0x02
};

struct RsTorManagerEvent: public RsEvent
{
    RsTorManagerEvent(): RsEvent(RsEventType::TOR_MANAGER),
            mTorManagerEventType(RsTorManagerEventCode::UNKNOWN)
    {}

    RsTorManagerEventCode mTorManagerEventType;

    RsTorConnectivityStatus mTorConnectivityStatus;
    RsTorStatus             mTorStatus;

    ///* @see RsEvent @see RsSerializable
    void serial_process( RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx ) override
    {
        RsEvent::serial_process(j, ctx);
        RS_SERIAL_PROCESS(mTorManagerEventType);
        RS_SERIAL_PROCESS(mTorConnectivityStatus);
        RS_SERIAL_PROCESS(mTorStatus);
    }

    ~RsTorManagerEvent() = default;
};

class RsTor
{
public:
    /*!
     * \brief isTorAvailable
     * \return true if a Tor executble has been found. False otherwise.
     */
    static bool isTorAvailable() ;

    /*!
     * \brief torStatus
     * \return Status of the Tor service used by RS
     */
    static RsTorStatus torStatus() ;

    /*!
     * \brief torConnectivityStatus
     * \return  Status of the connectivity/authentication between RS and Tor
     */
    static RsTorConnectivityStatus torConnectivityStatus() ;

    static void setTorDataDirectory(const std::string& dir);
    static void setHiddenServiceDirectory(const std::string& dir);

    static bool setupHiddenService();

    /*!
     * \brief getProxyServerInfo
     * \param server_address Address of the proxy used by RS to send data in the Tor network. Usually 127.0.0.1
     * \param server_port    Port of the proxy used by RS to send data in the Tor network. Usually 9050.
     */
    static void getProxyServerInfo(std::string& server_address,  uint16_t& server_port);

    /*!
     * \brief getHiddenServiceStatus
     * \param service_id onion address of the hidden service (if the service is OFFLINE or ONLINE)
     * \return Status of the created/loaded hidden service.
     */
    static RsTorHiddenServiceStatus getHiddenServiceStatus(std::string& service_id);

    /*!
     * \brief start
     * 			Launches the Tor management threads.
     */
    static bool start();

    /*!
     * \brief getHiddenServiceInfo
     * 			Gets information about the hidden service setup by RS to run.
     * \param service_id
     * \param service_onion_address
     * \param service_port
     * \param service_target_address
     * \param target_port
     * \return
     */
    static bool getHiddenServiceInfo(std::string& service_id,
                                     std::string& service_onion_address,
                                     uint16_t& service_port,
                                     std::string& service_target_address,
                                     uint16_t& target_port);

    /*!
     * \brief bootstrapStatus
     * \return Log messages of the Tor bootstrapping status.
     */
    static std::map<std::string,std::string> bootstrapStatus();
    static std::list<std::string> logMessages();

    static std::string socksAddress();
    static uint16_t socksPort();

    static bool hasError();
    static std::string errorMessage();

private:
    static Tor::TorManager *instance();
};
