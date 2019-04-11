/*******************************************************************************
 * libresapi/api/ApiServerMHD.h                                                *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <microhttpd.h>
#include <string>

#ifndef WINDOWS_SYS
#include <netinet/in.h>
#endif

namespace resource_api{
class ApiServer;

// returns the default docroot path
// (it is differen on different operating systems)
std::string getDefaultDocroot();

class ApiServerMHD
{
public:
    ApiServerMHD(ApiServer* server);
    ~ApiServerMHD();
    /**
     * @brief configure the http server
     * @param docroot sets the directory from which static files should be served. default = ./
     * @param port the port to listen on. The server will listen on ipv4 and ipv6.
     * @param bind_address NOT IMPLEMENTED optional, specifies an ipv6 adress to listen on.
     * @param allow_from_all when true, listen on all ips. (only when bind_adress is empty)
     * @return true on success
     */
    bool configure(std::string docroot, uint16_t port, std::string bind_address, bool allow_from_all);
    bool start();
    void stop();

private:
    // static callbacks for libmicrohttpd, they call the members below
    static int static_acceptPolicyCallback(void* cls, const struct sockaddr * addr, socklen_t addrlen);
    static int static_accessHandlerCallback(void* cls, struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    static void static_requestCompletedCallback(void *cls, struct MHD_Connection* connection, void **con_cls, enum MHD_RequestTerminationCode toe);
    int acceptPolicyCallback(const struct sockaddr * addr, socklen_t addrlen);
    int accessHandlerCallback(struct MHD_Connection * connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
    void requestCompletedCallback(struct MHD_Connection *connection, void **con_cls, MHD_RequestTerminationCode toe);
    bool mConfigOk;
    std::string mRootDir;
    struct sockaddr_in mListenAddr;
    MHD_Daemon* mDaemon;
    ApiServer* mApiServer;
};

} // namespace resource_api
