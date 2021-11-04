/*******************************************************************************
 * retroshare-gui/src/gui/NetworkDialog/pgpid_item_model.cpp                   *
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

#include "pgpid_item_model.h"
#include <time.h>
#include <retroshare/rspeers.h>
#include <QIcon>
#include <QBrush>


/*TODO:
 * using list here for internal data storage is not best option
*/
pgpid_item_model::pgpid_item_model(std::list<RsPgpId> &neighs_, float &_font_height,  QObject *parent)
    : QAbstractTableModel(parent), neighs(neighs_), font_height(_font_height)
{
}

QVariant pgpid_item_model::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case COLUMN_CHECK:
                return QString(tr(" Do you accept connections signed by this profile?"));
                break;
            case COLUMN_PEERNAME:
                return QString(tr("Name of the profile"));
                break;
            case COLUMN_I_AUTH_PEER:
                return QString(tr("This column indicates the trust level you indicated and whether you signed the profile PGP key"));
                break;
            case COLUMN_PEER_AUTH_ME:
                return QString(tr("Did that peer sign your own profile PGP key"));
                break;
            case COLUMN_PEERID:
                return QString(tr("PGP Key Id of that profile"));
                break;
            case COLUMN_LAST_USED:
                return QString(tr("Last time this key was used (received time, or to check connection)"));
                break;
            }
        }
        else if(role == Qt::DisplayRole)
        {
            switch(section)
            {
            case COLUMN_CHECK:
                return QString(tr("Connections"));
                break;
            case COLUMN_PEERNAME:
                return QString(tr("Profile"));
                break;
            case COLUMN_I_AUTH_PEER:
                return QString(tr("Trust level"));
                break;
            case COLUMN_PEER_AUTH_ME:
                return QString(tr("Has signed your key?"));
                break;
            case COLUMN_PEERID:
                return QString(tr("Id"));
                break;
            case COLUMN_LAST_USED:
                return QString(tr("Last used"));
                break;
            }
        }
        else if (role == Qt::TextAlignmentRole)
        {
            switch(section)
            {
            default:
                return (uint32_t)(Qt::AlignHCenter | Qt::AlignVCenter);
                break;
            }
        }
        else if(role == Qt::SizeHintRole)
        {
            switch(section)
            {
            case COLUMN_CHECK:
                return 25*font_height;
                break;
            case COLUMN_PEERNAME: case COLUMN_I_AUTH_PEER: case COLUMN_PEER_AUTH_ME:
                return 200*font_height;
                break;
            case COLUMN_LAST_USED:
                return 75*font_height;
                break;
            }

        }
    }
    return QVariant();
}


int pgpid_item_model::rowCount(const QModelIndex &/*parent*/) const
{
    return neighs.size();
}

int pgpid_item_model::columnCount(const QModelIndex &/*parent*/) const
{
    return COLUMN_COUNT;
}


QVariant pgpid_item_model::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if((unsigned)index.row() >= neighs.size())
        return QVariant();


    //shit code (please rewrite it)

    std::list<RsPgpId>::iterator it = neighs.begin();
    for(int i = 0; i < index.row(); i++)
        it++;
    RsPeerDetails detail;
    if (!rsPeers->getGPGDetails(*it, detail))
        return QVariant();
    //shit code end
    if(role == Qt::EditRole) //some columns return raw data for editrole, used for proper filtering and sorting
    {
        switch(index.column())
        {
        case COLUMN_LAST_USED:
            return detail.lastUsed;
            break;
        case COLUMN_I_AUTH_PEER:
        {
            if (detail.ownsign)
                return RS_TRUST_LVL_ULTIMATE;
            return detail.trustLvl;
        }
            break;
        case COLUMN_PEER_AUTH_ME:
            return detail.hasSignedMe;
            break;
        case COLUMN_CHECK:
            return detail.accept_connection;
            break;
        default:
            break;
        }

    }
    //we using editrole only where it is useful, for other data we use display, so no "else if" here
    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        switch(index.column())
        {
        case COLUMN_PEERNAME:
            return QString::fromUtf8(detail.name.c_str());
            break;
        case COLUMN_PEERID:
            return QString::fromStdString(detail.gpg_id.toStdString());
            break;
        case COLUMN_I_AUTH_PEER:
        {
            if (detail.ownsign)
                return tr("Personal signature");
            else
            {
                switch(detail.trustLvl)
                {
                    case RS_TRUST_LVL_MARGINAL: return tr("Marginally trusted peer") ; break;
                    case RS_TRUST_LVL_FULL:
                    case RS_TRUST_LVL_ULTIMATE: return tr("Fully trusted peer") ; break ;
                    case RS_TRUST_LVL_UNKNOWN:
                    case RS_TRUST_LVL_UNDEFINED:
                    case RS_TRUST_LVL_NEVER:
                    default: 							return tr("Untrusted peer") ; break ;
                }
            }
        }
            break;
        case COLUMN_PEER_AUTH_ME:
        {
            if (detail.hasSignedMe)
                return tr("Yes");
            else
                return tr("No");
        }
            break;
        case COLUMN_LAST_USED:
        {
            time_t now = time(NULL);
            uint64_t last_time_used = now - detail.lastUsed ;
            QString lst_used_str ;

            if(last_time_used < 3600)
                lst_used_str = tr("Last hour") ;
            else if(last_time_used < 86400)
                lst_used_str = tr("Today") ;
            else if(last_time_used > 86400 * 15000)
                lst_used_str = tr("Never");
            else
                lst_used_str = tr("%1 days ago").arg((int)( last_time_used / 86400 )) ;


            return lst_used_str;
        }
            break;
        case COLUMN_CHECK:
        {
            if (detail.accept_connection || rsPeers->getGPGOwnId() == detail.gpg_id)
                return tr("Accepted");
			else
                return tr("Denied");
        }
            break;


        }
    }
    else if(role == Qt::ToolTipRole)
    {
        switch(index.column())
        {
        case COLUMN_I_AUTH_PEER:
        {
            if (detail.ownsign)
                return tr("PGP key signed by you");
        }
            break;
        default:
        {
            if (!detail.accept_connection && detail.hasSignedMe)
            {
                return QString::fromUtf8(detail.name.c_str()) + tr(" has authenticated you. \nRight-click and select 'make friend' to be able to connect.");
            }
        }
            break;

        }
    }
    else if(role == Qt::DecorationRole)
    {
        switch(index.column())
        {
        case COLUMN_CHECK:
        {
            if (detail.accept_connection)
                return QIcon(IMAGE_AUTHED);
            else
                return QIcon(IMAGE_DENIED);

        }
            break;
        }
    }
    else if(role == Qt::BackgroundRole)
    {
        if (detail.accept_connection)
        {
            if (detail.ownsign)
            {
                return QBrush(mBackgroundColorOwnSign);
            }
            else
            {
                return QBrush(mBackgroundColorAcceptConnection);
            }
        }
        else
        {
            if (detail.hasSignedMe)
            {
                return QBrush(mBackgroundColorHasSignedMe);
            }
            else
            {
                return QBrush(mBackgroundColorDenied);
            }
        }
    }
    else if(role == Qt::ForegroundRole)
    {
        return QBrush(mTextColor);
    }
    return QVariant();
}


//following code is just a poc, it's still suboptimal, unefficient, but much better then existing rs code

void pgpid_item_model::data_updated(std::list<RsPgpId> &new_neighs)
{

    //shit code follow (rewrite this please)
    size_t old_size = neighs.size(), new_size = 0;
    std::list<RsPgpId> old_neighs = neighs;

    new_size = new_neighs.size();
    //set model data to new cleaned up data
    neighs = new_neighs;
    neighs.sort();
    neighs.unique(); //remove possible dups

    //reflect actual row count in model
    if(old_size < new_size)
    {
        beginInsertRows(QModelIndex(), old_size, new_size);
        insertRows(old_size, new_size - old_size);
        endInsertRows();
    }
    else if(new_size < old_size)
    {
        beginRemoveRows(QModelIndex(), new_size, old_size);
        removeRows(old_size, old_size - new_size);
        endRemoveRows();
    }
    //update data in ui, to avoid unnecessary redraw and ui updates, updating only changed elements
    //TODO: libretroshare should implement a way to obtain only changed elements via some signalling non-blocking api.
    {
        size_t ii1 = 0;
        for(auto i1 = neighs.begin(), end1 = neighs.end(), i2 = old_neighs.begin(), end2 = old_neighs.end(); i1 != end1; ++i1, ++i2, ii1++)
        {
            if(i2 == end2)
                break;
            if(*i1 != *i2)
            {
                QModelIndex topLeft = createIndex(ii1,0), bottomRight = createIndex(ii1, COLUMN_COUNT-1);
                emit dataChanged(topLeft, bottomRight);
            }
        }
    }
    if(new_size > old_size)
    {
        QModelIndex topLeft = createIndex(old_size ? old_size -1 : 0 ,0), bottomRight = createIndex(new_size -1, COLUMN_COUNT-1);
        emit dataChanged(topLeft, bottomRight);
    }
    //dirty solution for initial data fetch
    //TODO: do it properly!
    if(!old_size)
    {
        beginResetModel();
        endResetModel();
    }

    //shit code end
}

