#include "pgpid_item_model.h"
#include <retroshare/rspeers.h>
#include <QIcon>

#define IMAGE_AUTHED         ":/images/accepted16.png"
#define IMAGE_DENIED         ":/images/denied16.png"
#define IMAGE_TRUSTED        ":/images/rs-2.png"

/*TODO:
 * using list here for internal data storage is not best options
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
                return QString(tr("This column indicates trust level and whether you signed the profile PGP key"));
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
                return QString(tr(""));
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

/*QModelIndex pgpid_item_model::index(int row, int column, const QModelIndex &parent) const
{
    return createIndex(row, column);
}*/

/*QModelIndex pgpid_item_model::parent(const QModelIndex &index) const
{
    if(index.row() > -1 && index.column() > -1)
        return createIndex(-1, -1);
    if (!index.isValid())
        return QModelIndex();
    return QModelIndex();
}*/

/*bool pgpid_item_model::hasChildren(const QModelIndex &parent) const
{
    if(parent.column() == -1 && parent.row() == -1)
        return true;
    return false;
} */

int pgpid_item_model::rowCount(const QModelIndex &/*parent*/) const
{
    return neighs.size();
}

int pgpid_item_model::columnCount(const QModelIndex &/*parent*/) const
{
    return COLUMN_COUNT;
}

//bool pgpid_item_model::insertRows(int position, int rows, const QModelIndex &/*index*/)
//{
//    beginInsertRows(QModelIndex(), position, position+rows-1);
//    endInsertRows();
//    return true;
//}

//bool pgpid_item_model::removeRows(int position, int rows, const QModelIndex &/*index*/)
//{
//     beginRemoveRows(QModelIndex(), position, position+rows-1);
//     endRemoveRows();
//     return true;
//}


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
    if(role == Qt::DisplayRole)
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

//            QString lst_used_sort_str = QString::number(detail.lastUsed,'f',10);

            return lst_used_str;
//            item->setData(COLUMN_LAST_USED,ROLE_SORT,lst_used_sort_str) ;
        }
            break;
        case COLUMN_CHECK:
        {
            if (detail.accept_connection)
            {
                return QString("0");
            }
            else
            {
                return QString("1");
            }
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
        switch(index.column())
        {
        default:
        {
            //TODO: add access to bckground colors from networkdialog
            if (detail.accept_connection)
            {
                if (detail.ownsign)
                    ;
            }
        }
            break;

        }
    }


    return QVariant();
}

/*void pgpid_item_model::sort(int column, Qt::SortOrder order)
{

} */


//following code is just a poc, it's still suboptimal, unefficient, but much better then existing rs code

void pgpid_item_model::data_updated(std::list<RsPgpId> &new_neighs)
{

    //shit code follow (rewrite this please)
    size_t old_size = neighs.size(), new_size = 0;
    std::list<RsPgpId> old_neighs = neighs;

    //find all bad elements in list
    std::list<RsPgpId> bad_elements;
    for(std::list<RsPgpId>::iterator it = new_neighs.begin(); it != new_neighs.end(); ++it)
    {
        if (*it == rsPeers->getGPGOwnId())
            bad_elements.push_back(*it);
        RsPeerDetails detail;
        if (!rsPeers->getGPGDetails(*it, detail))
            bad_elements.push_back(*it);

    }
    //remove all  bad elements from list
    for(std::list<RsPgpId>::iterator it = bad_elements.begin(); it != bad_elements.end(); ++it)
    {
        std::list<RsPgpId>::iterator it2 = std::find(new_neighs.begin(), new_neighs.end(), *it);
        if(it2 != new_neighs.end())
            new_neighs.remove(*it2);
    }
    new_size = new_neighs.size();
    //set model data to new cleaned up data
    neighs = new_neighs;

    //reflect actual row count in model
    if(old_size < new_size)
    {
        beginInsertRows(QModelIndex(), old_size - 1, old_size - 1 + new_size - old_size);
        insertRows(old_size - 1 , new_size - old_size);
        endInsertRows();
    }
    else if(new_size < old_size)
    {
        beginRemoveRows(QModelIndex(), new_size - 1, new_size - 1 + old_size - new_size);
        removeRows(old_size - 1, old_size - new_size);
        endRemoveRows();
    }
    //update data in ui, to avoid unnecessary redraw and ui updates, updating only changed elements
    //i guessing what order is unchanged between rsPeers->getGPGAllList() calls
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

