/*******************************************************************************
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

#include "serialiser/rsserializable.h"

#include <iostream>

std::ostream& operator<<(std::ostream& out, const RsSerializable& serializable)
{
	RsGenericSerializer::SerializeContext ctx;
	const_cast<RsSerializable&>(serializable) // safe with TO_JSON
	        .serial_process(RsGenericSerializer::TO_JSON, ctx);
	return out << ctx.mJson;
}
