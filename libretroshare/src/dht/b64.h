/*
 * libretroshare/src/dht: b64.h
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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


#ifndef BASE64_CODE_H
#define BASE64_CODE_H

#include <string>

std::string convertToBase64(std::string input);
std::string convertFromBase64(std::string input);


uint32_t DataLenFromBase64(std::string input);
std::string convertDataToBase64(unsigned char *data, uint32_t dlen);
bool convertDataFromBase64(std::string input, unsigned char *data, uint32_t *dlen);

		
#endif

