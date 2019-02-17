/*******************************************************************************
 * gui/common/PeerDefs.h                                                       *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _PEERDEFS_H
#define _PEERDEFS_H

#include <QString>
#include <retroshare/rsidentity.h>

struct RsPeerDetails;

class PeerDefs
{
public:
    static const QString nameWithLocation(const RsPeerDetails &details);
    static const QString nameWithLocation(const RsIdentityDetails &details);
    static const QString nameWithId(const RsIdentityDetails &details);

    static const QString rsid(const RsPeerDetails &details);
    static const QString rsid(const std::string &name, const RsPeerId &id);
    static const QString rsid(const std::string &name, const RsPgpId &id);
    static const QString rsid(const std::string &name, const RsGxsId &id);
    static const QString rsidFromId(const RsPgpId &id, QString *name = NULL);
    static const QString rsidFromId(const RsPeerId &id, QString *name = NULL);
    static const QString rsidFromId(const RsGxsId &id, QString *name = NULL);
    static RsPeerId idFromRsid(const QString &rsid, bool check);
};

#endif

