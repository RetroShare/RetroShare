/*******************************************************************************
 * util/RsNetUtil.cpp                                                          *
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

#include <QCoreApplication>
#include <QStringList>

#include "RsNetUtil.h"

#include <iostream>

#ifdef WINDOWS_SYS
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

bool RsNetUtil::parseAddrFromQString(const QString& s, struct sockaddr_storage& addr, int& bytes)
{
	QStringList lst = s.split(".") ;
	bytes = 0 ;

	memset(&addr,0,sizeof(sockaddr_storage)) ;

	addr.ss_family = AF_INET ;

	QStringList::const_iterator it = lst.begin();
	bool ok ;

	uint32_t s1 = (*it).toInt(&ok) ; ++it ; if(!ok) return false ; if(it ==lst.end()) return false ;
	uint32_t s2 = (*it).toInt(&ok) ; ++it ; if(!ok) return false ; if(it ==lst.end()) return false ;
	uint32_t s3 = (*it).toInt(&ok) ; ++it ; if(!ok) return false ; if(it ==lst.end()) return false ;

	QStringList lst2 = (*it).split("/") ;

	it = lst2.begin();

	uint32_t s4 ;
	s4 = (*it).toInt(&ok) ; ++it ; if(!ok) return false ;

	if(it != lst2.end())
	{
		uint32_t x = (*it).toInt(&ok) ; if(!ok) return false ;
		if(x%8 != 0)
			return false ;

        if(x != 16 &&  x != 24 && x != 32)
			return false ;

		bytes = 4 - x/8 ;
	}

	const sockaddr_in *in = (const sockaddr_in*)&addr ;
	((uint8_t*)&in->sin_addr.s_addr)[0] = s1 ;
	((uint8_t*)&in->sin_addr.s_addr)[1] = s2 ;
	((uint8_t*)&in->sin_addr.s_addr)[2] = s3 ;
	((uint8_t*)&in->sin_addr.s_addr)[3] = s4 ;

	return true;
}

QString RsNetUtil::printAddr(const struct sockaddr_storage& addr)
{
	const sockaddr_in *in = (const sockaddr_in*)&addr ;

	uint8_t *bytes = (uint8_t *) &(in->sin_addr.s_addr);
	return QString("%1.%2.%3.%4").arg(bytes[0]).arg(bytes[1]).arg(bytes[2]).arg(bytes[3]);
}

QString RsNetUtil::printAddrRange(const struct sockaddr_storage& addr, uint8_t masked_bytes)
{
	const sockaddr_in *in = (const sockaddr_in*)&addr ;

	uint8_t *bytes = (uint8_t *) &(in->sin_addr.s_addr);

	switch(masked_bytes)
	{
	case 0:
		return QString("%1.%2.%3.%4").arg(bytes[0]).arg(bytes[1]).arg(bytes[2]).arg(bytes[3]);
	case 1:
		return QString("%1.%2.%3.255/24").arg(bytes[0]).arg(bytes[1]).arg(bytes[2]);
	case 2:
		return QString("%1.%2.255.255/16").arg(bytes[0]).arg(bytes[1]);
	}

	std::cerr << "ERROR: Wrong format : masked_bytes = " << masked_bytes << std::endl;
	return QCoreApplication::translate("RsNetUtil", "Invalid format");
}
