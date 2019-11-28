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
#pragma once

#include <iostream>
#include <rapidjson/document.h>

/**
 * Use this type for JSON documents representations in RetroShare code
 */
typedef rapidjson::Document RsJson;

/**
 * Print out RsJson to a stream, use std::stringstream to get the string
 * @param[out] out output stream
 * @param[in] jDoc JSON document to print
 * @return same output stream passed as out parameter
 */
std::ostream& operator<<(std::ostream &out, const RsJson &jDoc);

/**
 * Stream manipulator to print RsJson in compact format
 * @param[out] out output stream
 * @return same output stream passed as out parameter
 */
std::ostream& compactJSON(std::ostream &out);

/**
 * Stream manipulator to print RsJson in human readable format
 * @param[out] out output stream
 * @return same output stream passed as out parameter
 */
std::ostream& prettyJSON(std::ostream &out);
