/*******************************************************************************
 * libretroshare/src/pqi: pqi.h                                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef PQI_TOP_HEADER
#define PQI_TOP_HEADER

#include "rsitems/rsitem.h"

class P3Interface
{
public:
	P3Interface() {return; }
virtual ~P3Interface() {return; }

virtual int	tick() { return 1; }
virtual int	status() { return 1; }

virtual int	SendRsRawItem(RsRawItem *) = 0;
};


/**
 * @brief Interface to allow outgoing messages to be sent directly through to
 * the pqiperson, rather than being queued
 */
class pqiPublisher
{
        public:
virtual ~pqiPublisher() { return; }
virtual bool sendItem(RsRawItem *item) = 0;

};


#endif // PQI_TOP_HEADER
