/*
 * libretroshare/src/pqi: cleanupxpgp.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2008 by Sourashis Roy
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef PQI_XPGP_CLEANUP_H
#define PQI_XPGP_CLEANUP_H

#include <string>

//! Converts invitation to clean XPGP certificate

//!  This function was used for extracting XPGP certificates  from invitation
//!letters. Typical input was something like
//! You have been invited to join the retroshare community by: beardog-unstable-02
//!   
//! Retroshare is a Friend-2-Friend network that enables you to communicate securely and privately ....
//! ...  text stuff .....
//!
//!-----BEGIN XPGP CERTIFICATE-----
//!Version: ...
//!
//!MIICxQIBADCCAUkCAQAwHhcNMDkwMjI4MTgzODIyWhcNMTQwMjI3MTgzODIyWjCC
//! ...more ines here...
//!mEuhG8UmDIzC1jeTu8rTMnO+DO3FH/cek1vlfFl4t9g/xktG9U4SPLg=
//!=checksum
//!-----END XPGP CERTIFICATE-----
//!  
//! In the newer gui version, users send each other almost clean certificates,
//! so this functon is used only to avoid possible bugs with line endings
std::string cleanUpCertificate(const std::string& badCertificate);

#endif
