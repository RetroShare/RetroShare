/*******************************************************************************
 * libretroshare/src/pqi: pqinetwork.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <vector>

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/poll.h>
#include <errno.h>

//socket blocking/options.
#include <fcntl.h>
#include <inttypes.h>

#else

#include "util/rsnet.h" /* more generic networking header */

// Some Network functions that are missing from windows.
int inet_aton(const char *name, struct in_addr *addr);

extern int errno; /* Define extern errno, to duplicate unix behaviour */

/* define the Unix Error Codes that we use...
 * NB. we should make the same, but not necessary
 */
#define EAGAIN          11

#define EUSERS          87

#define EHOSTDOWN       112

#ifndef __MINGW64_VERSION_MAJOR
#define EWOULDBLOCK     EAGAIN

#define ENOTSOCK        88

#define EOPNOTSUPP      95

#define EADDRINUSE      98
#define EADDRNOTAVAIL   99
#define ENETDOWN        100
#define ENETUNREACH     101

#define ECONNRESET      104

#define ETIMEDOUT       10060 // value from pthread.h
#define ECONNREFUSED    111
#define EHOSTUNREACH    113
#define EALREADY        114
#define EINPROGRESS     115
#endif

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

#include <iostream>
#include <string>
#include <list>

#include "util/rsdeprecate.h"

// Same def - different functions...
RS_DEPRECATED_FOR("use std::error_condition instead")
void showSocketError(std::string &out);

RS_DEPRECATED_FOR("use std::error_condition instead")
std::string socket_errorType(int err);

bool getLocalAddresses(std::vector<sockaddr_storage> & addrs);

/* universal socket interface */

int unix_close(int sockfd);
int unix_socket(int domain, int type, int protocol);
int unix_fcntl_nonblock(int sockfd);
int unix_connect(int sockfd, const sockaddr_storage& serv_addr);
int unix_getsockopt_error(int sockfd, int *err);

#ifdef WINDOWS_SYS // WINDOWS
/******************* WINDOWS SPECIFIC PART ******************/
RS_DEPRECATED_FOR("use std::error_condition instead")
int     WinToUnixError(int error);
#endif
