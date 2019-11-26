/*******************************************************************************
 * libretroshare/src/rsserver: rswebui.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2019-2019 Cyril Soler <csoler@users.sourceforge.net>              *
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

class RsWebUI
{
public:
    enum {
        WEBUI_STATUS_UNKNOWN     = 0x00,
        WEBUI_STATUS_NOT_RUNNING = 0x01,
        WEBUI_STATUS_RUNNING     = 0x02
    };
    static const std::string DEFAULT_BASE_DIRECTORY ;

    virtual bool restart() =0;
    virtual bool stop()  =0;

    virtual void setHtmlFilesDirectory(const std::string& html_dir) =0;
    virtual void setUserPassword(const std::string& passwd) =0;

    virtual int status() const=0;
};

extern RsWebUI *rsWebUI;
