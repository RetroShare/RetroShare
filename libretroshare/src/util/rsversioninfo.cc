/*******************************************************************************
 * libretroshare/src/util: rsversioninfo.cc                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2013 by  Alexandrut                                          *
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
#include "rsversioninfo.h"
#include "retroshare/rsversion.h"
#include "rsstring.h"

std::string RsUtil::retroshareVersion()
{
	std::string version;
	rs_sprintf(version, "%d.%d.%d%s Revision %08x", RS_MAJOR_VERSION, RS_MINOR_VERSION, RS_BUILD_NUMBER, RS_BUILD_NUMBER_ADD, RS_REVISION_NUMBER);

	return version;
}

uint32_t RsUtil::retroshareRevision()
{
	return RS_REVISION_NUMBER;
}
