/*******************************************************************************
 * retroshare-gui/src/gui/NetworkDialog/pgpid_item_proxy.cpp                   *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include "pgpid_item_proxy.h"

//TODO: include only required headers here
#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsmsgs.h>


//TODO: set this defines in one place
// Defines for key list columns
#define COLUMN_CHECK 0
#define COLUMN_PEERNAME    1
#define COLUMN_I_AUTH_PEER 2
#define COLUMN_PEER_AUTH_ME 3
#define COLUMN_PEERID      4
#define COLUMN_LAST_USED   5
#define COLUMN_COUNT 6



pgpid_item_proxy::pgpid_item_proxy(QObject *parent) :
    QSortFilterProxyModel(parent)
{

}

void pgpid_item_proxy::use_only_trusted_keys(bool val)
{
    only_trusted_keys = val;
    filterChanged();
}

bool pgpid_item_proxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if(only_trusted_keys)
    {
        if(!rsPeers)
            return false;
        RsPgpId peer_id (sourceModel()->data(sourceModel()->index(sourceRow, COLUMN_PEERID, sourceParent)).toString().toStdString());
        RsPeerDetails details;
        if(!rsPeers->getGPGDetails(peer_id, details))
            return false;
        if(details.validLvl < RS_TRUST_LVL_MARGINAL)
            return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
