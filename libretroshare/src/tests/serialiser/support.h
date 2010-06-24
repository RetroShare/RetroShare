#ifndef SUPPORT_H_
#define SUPPORT_H_

/*
 * libretroshare/src/tests/serialiser:
 *
 * RetroShare Serialiser tests.
 *
 * Copyright 2007-2008 by Christopher Evi-Parker
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
#include <stdint.h>

void randString(const uint32_t, std::string&);
void randString(const uint32_t, std::wstring&);

#endif /* SUPPORT_H_ */
