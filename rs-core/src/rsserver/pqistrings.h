#ifndef PQI_STRINGS_H
#define PQI_STRINGS_H

/*
 * "$Id: pqistrings.h,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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


#include <string>
#include <list>

class Person;
class cert;

#include <openssl/ssl.h>

std::string get_cert_country(cert *c);
std::string get_cert_loc(cert *c);
std::string get_cert_org(cert *c);
std::string get_cert_name(cert *c);

std::string get_status_string(int status);
std::string get_autoconnect_string(Person *p);
std::string get_server_string(Person *p);
std::string get_trust_string(Person *p);
std::string get_timeperiod_string(int secs);

std::string get_cert_info(cert *c);
std::string get_neighbour_info(cert *c);

int get_lastconnecttime(Person *p);
std::string get_lastconnecttime_string(Person *p);

std::string getX509NameString(X509_NAME *name);
std::string getX509Info(X509 *cert);
std::string getX509CNString(X509_NAME *name);

#define TIME_FORMAT_BRIEF		0x001
#define TIME_FORMAT_LONG		0x002
#define TIME_FORMAT_NORMAL		0x003
#define TIME_FORMAT_OLDVAGUE		0x004
#define TIME_FORMAT_OLDVAGUE_NOW	0x005
#define TIME_FORMAT_OLDVAGUE_WEEK	0x006
#define TIME_FORMAT_OLDVAGUE_OLD	0x007

std::string timeFormat(int epoch, int format);

#if defined(PQI_USE_XPGP)

std::string getXPGPInfo(XPGP *cert);
std::string getXPGPAuthCode(XPGP *xpgp);
std::list<std::string> getXPGPsigners(XPGP *cert);

#endif /* XPGP Certificates */


#endif
