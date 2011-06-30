/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


#ifndef _PEERDEFS_H
#define _PEERDEFS_H

#include <QString>

class RsPeerDetails;

class PeerDefs
{
public:
    static const QString nameWithLocation(const RsPeerDetails &details);

    static const QString rsid(const RsPeerDetails &details);
    static const QString rsid(const std::string &name, const std::string &id);
    static const QString rsidFromId(const std::string &id, QString *name = NULL);
    static const std::string idFromRsid(const QString &rsid, bool check);
};

#endif

