/*******************************************************************************
 * libretroshare/src/retroshare: rsjsonapi.h                                   *
 *                                                                             *
 * Copyright (C) 2019-2019  Cyril Soler <csoler@users.sourceforge.net>         *
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
          *
 *******************************************************************************/
#pragma once

class RsJsonAPI
{
public:
	static const uint16_t    DEFAULT_PORT = 9092 ;
	static const std::string DEFAULT_BINDING_ADDRESS ;	// 127.0.0.1

	virtual bool restart() =0;
	virtual bool stop()  =0;

	virtual void setHtmlFilesDirectory(const std::string& html_dir) =0;
	virtual void setListeningPort(uint16_t port) =0;

	virtual int status() const=0;
};

extern RsJsonAPI *rsJsonAPI;

