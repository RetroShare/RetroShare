/*
 * libretroshare/src/serialiser: t_support.h.cc
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

#include <stdlib.h>

#include "support.h"



void randString(const uint32_t length, std::string& outStr)
{
	char alpha = 'a';
	char* stringData = NULL;

	stringData = new char[length];

	for(int i=0; i != length; i++)
		stringData[i] = alpha + (rand() % 26);

	outStr.assign(stringData, length);
	delete[] stringData;

	return;
}

void randString(const uint32_t length, std::wstring& outStr)
{
	wchar_t alpha = L'a';
	wchar_t* stringData = NULL;

	stringData = new wchar_t[length];

	for(int i=0; i != length; i++)
		stringData[i] = (alpha + (rand() % 26));

	outStr.assign(stringData, length);
	delete[] stringData;

	return;
}
