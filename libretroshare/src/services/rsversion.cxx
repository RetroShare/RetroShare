/*******************************************************************************
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "retroshare/rsversion.h"

/*extern*/ RsVersion* rsVersion = new RsVersion;

/*static*/ void RsVersion::version(
        uint32_t& major, uint32_t& minor, uint32_t& mini, std::string& extra,
        std::string& human )
{
	major = RS_MAJOR_VERSION;
	minor = RS_MINOR_VERSION;
	mini = RS_MINI_VERSION;
	extra = RS_EXTRA_VERSION;
	human = RS_HUMAN_READABLE_VERSION;
}
