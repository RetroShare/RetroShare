/*******************************************************************************
 * libretroshare/src/pqi: pqissli2pbob.h                                       *
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
#ifndef PQISSLI2PBOB_H
#define PQISSLI2PBOB_H

#include "pqi/pqissl.h"

/*
 * This class is a minimal varied version of pqissl to work with I2P BOB tunnels.
 * The only difference is that the [.b32].i2p addresses must be sent first.
 *
 * Everything else is untouched.
 */

class pqissli2pbob : public pqissl
{
public:
	pqissli2pbob(pqissllistener *l, PQInterface *parent, p3LinkMgr *lm)
	    : pqissl(l, parent, lm) {}

	// NetInterface interface
public:
	bool connect_parameter(uint32_t type, const std::string &value);

	// pqissl interface
protected:
	int Basic_Connection_Complete();

private:
	std::string mI2pAddr;
};

#endif // PQISSLI2PBOB_H
