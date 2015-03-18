#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <microhttpd.h>
#include <string>

#include "api/ApiServer.h"

namespace resource_api{

class ApiServerMHD
{
public:
    ApiServerMHD(std::string root_dir, uint16_t port);
    ~ApiServerMHD();
    bool start();
    void stop();

    ApiServer& getApiServer(){ return mApiServer; }


    // internal helper
    static void sendMessage(struct MHD_Connection* connection, unsigned int status, std::string message);
private:
    // static callbacks for libmicrohttpd, they call the members below
    static int static_acceptPolicyCallback(void* cls, const struct sockaddr * addr, socklen_t addrlen);
    static int static_accessHandlerCallback(void* cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    static void static_requestCompletedCallback(void *cls, struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe);
    int acceptPolicyCallback(const struct sockaddr * addr, socklen_t addrlen);
    int accessHandlerCallback(struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    void requestCompletedCallback(struct MHD_Connection *connection, void **con_cls, MHD_RequestTerminationCode toe);
    std::string mRootDir;
    uint16_t mPort;
    MHD_Daemon* mDaemon;
    ApiServer mApiServer;
};

} // namespace resource_api
