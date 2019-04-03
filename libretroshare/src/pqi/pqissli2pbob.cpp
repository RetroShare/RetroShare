/*******************************************************************************
 * libretroshare/src/pqi: pqissli2pbob.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Sehraf                                                    *
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
#include "pqissli2pbob.h"

bool pqissli2pbob::connect_parameter(uint32_t type, const std::string &value)
{
	if (type == NET_PARAM_CONNECT_DOMAIN_ADDRESS)
	{
		RS_STACK_MUTEX(mSslMtx);
		// a new line must be appended!
		mI2pAddr = value + '\n';
		return true;
	}

	return pqissl::connect_parameter(type, value);
}

int pqissli2pbob::Basic_Connection_Complete()
{
	int ret;

	if ((ret = pqissl::Basic_Connection_Complete()) != 1)
	{
		// basic connection not complete.
		return ret;
	}

	// send addr. (new line is already appended)
	ret = send(sockfd, mI2pAddr.c_str(), mI2pAddr.length(), 0);
	if (ret != (int)mI2pAddr.length())
		return -1;
	return 1;
}
