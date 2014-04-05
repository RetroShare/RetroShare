#ifndef RS_TLV_UTIL_H
#define RS_TLV_UTIL_H

/*
 * libretroshare/src/serialiser: rstlvutil.h
 *
 * RetroShare Serialiser.
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

/* some utility functions mainly for debugging 
 */

#include <inttypes.h>
#include <ostream>
#include <vector>

class RsTlvItem;

/* print out a packet */
void displayRawPacket(std::ostream &out, void *data, uint32_t size);
int test_SerialiseTlvItem(std::ostream &str, RsTlvItem *in, RsTlvItem *out);


bool test_StepThroughTlvStack(std::ostream &str, void *data, int size);
int test_CreateTlvStack(std::ostream &str,
                std::vector<RsTlvItem *> items, void *data, int totalsize);
int test_TlvSet(std::vector<RsTlvItem *> items, int maxsize);

#endif
