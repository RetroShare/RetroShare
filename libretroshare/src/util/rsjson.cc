/*******************************************************************************
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "util/rsjson.h"

#ifdef HAS_RAPIDJSON
#	include <rapidjson/writer.h>
#	include <rapidjson/stringbuffer.h>
#	include <rapidjson/prettywriter.h>
#else
#	include <rapid_json/writer.h>
#	include <rapid_json/stringbuffer.h>
#	include <rapid_json/prettywriter.h>
#endif // HAS_RAPIDJSON


inline int getJsonManipulatorStatePosition()
{
	static int p = std::ios_base::xalloc();
	return p;
}

std::ostream& compactJSON(std::ostream &out)
{
	out.iword(getJsonManipulatorStatePosition()) = 1;
	return out;
}

std::ostream& prettyJSON(std::ostream &out)
{
	out.iword(getJsonManipulatorStatePosition()) = 0;
	return out;
}

std::ostream& operator<<(std::ostream &out, const RsJson &jDoc)
{
	rapidjson::StringBuffer buffer; buffer.Clear();
	if(out.iword(getJsonManipulatorStatePosition()))
	{
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		jDoc.Accept(writer);
	}
	else
	{
		rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
		jDoc.Accept(writer);
	}

	return out << buffer.GetString();
}
