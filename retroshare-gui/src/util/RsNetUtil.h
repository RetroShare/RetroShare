/*******************************************************************************
 * util/RsNetUtil.h                                                            *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSNETUTIL_H
#define _RSNETUTIL_H

#include <QString>
#include <inttypes.h>
/* get OS-specific definitions for:
 * 	struct sockaddr_storage
 */
#ifndef WINDOWS_SYS
	#include <sys/socket.h>
#else
	#include <winsock2.h>
#endif

class RsNetUtil
{
public:
	static bool parseAddrFromQString(const QString& s, struct sockaddr_storage& addr, int& bytes);
	static QString printAddr(const struct sockaddr_storage& addr);
	static QString printAddrRange(const struct sockaddr_storage& addr, uint8_t masked_bytes);
};

#endif
