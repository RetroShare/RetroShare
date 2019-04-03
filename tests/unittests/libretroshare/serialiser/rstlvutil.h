/*******************************************************************************
 * unittests/libretroshare/serialiser/rstlvutil.h                              *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare.project@gmail.com>         *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/
#ifndef RS_TLV_UTIL_H
#define RS_TLV_UTIL_H

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
