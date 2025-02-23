/*******************************************************************************
 * retroshare-gui/src/gui/msgs/RsFriendListModel.cpp                           *
 *                                                                             *
 * Copyright 2019 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <list>

#include <QApplication>
#include <QDateTime>
#include <QFontMetrics>
#include <QModelIndex>
#include <QTreeView>
#include <QPainter>
#include <QIcon>

#include "gui/common/AvatarDefs.h"
#include "util/qtthreadsutils.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/gxs/GxsIdDetails.h"
#include "retroshare/rsexpr.h"

#include "IdentityListModel.h"

//#define DEBUG_MODEL
//#define DEBUG_MODEL_INDEX

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsIdentityListModel::FilterString("filtered");

const uint32_t MAX_INTERNAL_DATA_UPDATE_DELAY = 300 ; 	// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.
const uint32_t MAX_NODE_UPDATE_DELAY = 10 ; 			// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.

static const uint32_t NODE_DETAILS_UPDATE_DELAY = 5;	// update each node every 5 secs.

RsIdentityListModel::RsIdentityListModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mLastInternalDataUpdate(0), mLastNodeUpdate(0)
{
	mFilterStrings.clear();
    mIdentityUpdateTimer = new QTimer();
    connect(mIdentityUpdateTimer,SIGNAL(timeout()),this,SLOT(timerUpdate()));
}

void RsIdentityListModel::timerUpdate()
{
    std::cerr << "updating indices" << std::endl;
    emit dataChanged(index(0,0,QModelIndex()),index(2,0,QModelIndex()));
}
RsIdentityListModel::EntryIndex::EntryIndex()
   : type(ENTRY_TYPE_INVALID),category_index(0),identity_index(0)
{
}

// The index encodes the whole hierarchy of parents. This allows to very efficiently compute indices of the parent of an index.
//
// On 32 bits and 64 bits architectures the format is the following:
//
//     0x [2 bits] 00000 [24 bits] [2 bits]
//            |              |        |
//            |              |        +-------------- type (0=top level, 1=category, 2=identity)
//            |              +----------------------- identity index
//            +-------------------------------------- category index
//

bool RsIdentityListModel::convertIndexToInternalId(const EntryIndex& e,quintptr& id)
{
    // the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    if(e.type == ENTRY_TYPE_INVALID)
    {
        RsErr() << "ERROR: asked for the internal id of an invalid EntryIndex" ;
        id = 0;
        return true;
    }
    if(bool(e.identity_index >> 24))
    {
        RsErr() << "Cannot encode more than 2^24 identities. Somthing's wrong. e.identity_index = " << std::hex << e.identity_index << std::dec ;
        id = 0;
        return false;
    }

    id = ((0x3 & (uint32_t)e.category_index) << 30) + ((uint32_t)e.identity_index << 2) + (0x3 & (uint32_t)e.type);

    return true;
}
bool RsIdentityListModel::convertInternalIdToIndex(quintptr ref,EntryIndex& e)
{
    // Compatible with ref=0 since it will cause type=TOP_LEVEL

    e.type            = static_cast<RsIdentityListModel::EntryType>((ref >>  0) & 0x3) ;// 2 bits
    e.identity_index  = (ref >>  2) & 0xffffff;// 24 bits
    e.category_index  = (ref >> 30) & 0x3 ;// 2 bits

    return true;
}

static QIcon createAvatar(const QPixmap &avatar, const QPixmap &overlay)
{
	int avatarWidth = avatar.width();
	int avatarHeight = avatar.height();

	QPixmap pixmap(avatar);

	int overlaySize = (avatarWidth > avatarHeight) ? (avatarWidth/2.5) :  (avatarHeight/2.5);
	int overlayX = avatarWidth - overlaySize;
	int overlayY = avatarHeight - overlaySize;

	QPainter painter(&pixmap);
	painter.drawPixmap(overlayX, overlayY, overlaySize, overlaySize, overlay);

	QIcon icon;
	icon.addPixmap(pixmap);
	return icon;
}

void RsIdentityListModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsIdentityListModel::postMods()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,columnCount()-1,(void*)NULL));
	emit layoutChanged();
}

int RsIdentityListModel::rowCount(const QModelIndex& parent) const
{
	if(parent.column() >= COLUMN_THREAD_NB_COLUMNS)
		return 0;

    EntryIndex index;

    if(!parent.isValid() || !convertInternalIdToIndex(parent.internalId(),index))
        return mCategories.size();

    switch(index.type)
    {
    case ENTRY_TYPE_CATEGORY: return mCategories[index.category_index].child_identity_indices.size();
    case ENTRY_TYPE_TOP_LEVEL: return mCategories.size();
    default:
        return 0;
    }
}

int RsIdentityListModel::columnCount(const QModelIndex &/*parent*/) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

bool RsIdentityListModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

    EntryIndex parent_index ;
    convertInternalIdToIndex(parent.internalId(),parent_index);

    if(parent_index.type == ENTRY_TYPE_TOP_LEVEL)
        return true;

    if(parent_index.type == ENTRY_TYPE_IDENTITY)
        return false;

    if(parent_index.type == ENTRY_TYPE_CATEGORY)
        return !mCategories[parent_index.category_index].child_identity_indices.empty();

    return false;
}

RsIdentityListModel::EntryIndex RsIdentityListModel::EntryIndex::parent() const
{
	EntryIndex i(*this);

	switch(type)
	{
        case ENTRY_TYPE_CATEGORY: i.type = ENTRY_TYPE_TOP_LEVEL;
                                  i.category_index = 0;
                                  i.identity_index = 0;
        break;

        case ENTRY_TYPE_IDENTITY: 	i.type = ENTRY_TYPE_CATEGORY;
                                    i.identity_index = 0;
		break;
        case ENTRY_TYPE_TOP_LEVEL:
    default:
			//Can be when request root index.
		break;
	}

	return i;
}

RsIdentityListModel::EntryIndex RsIdentityListModel::EntryIndex::child(int row) const
{
    EntryIndex i;

	switch(type)
    {
    case ENTRY_TYPE_TOP_LEVEL:
                            i.type = ENTRY_TYPE_CATEGORY;
                           i.category_index = row;
                           i.identity_index = 0;

						   break;

    case ENTRY_TYPE_CATEGORY: i.type = ENTRY_TYPE_IDENTITY;
                           i.category_index = category_index;
                           i.identity_index = row;
						   break;

    case ENTRY_TYPE_IDENTITY:  i = EntryIndex();
    default:
						   break;
    }

    return i;

}
uint32_t   RsIdentityListModel::EntryIndex::parentRow() const
{
    switch(type)
    {
    default:
        case ENTRY_TYPE_TOP_LEVEL: return 0;
        case ENTRY_TYPE_CATEGORY : return category_index;
        case ENTRY_TYPE_IDENTITY : return identity_index;
    }
}

QModelIndex RsIdentityListModel::index(int row, int column, const QModelIndex& parent) const
{
    if(row < 0 || column < 0 || column >= columnCount(parent) || row >= rowCount(parent))
		return QModelIndex();

    EntryIndex parent_index ;
    convertInternalIdToIndex(parent.internalId(),parent_index);
#ifdef DEBUG_MODEL_INDEX
    RsDbg() << "Index row=" << row << " col=" << column << " parent=" << parent << std::endl;
#endif

    quintptr ref;
    EntryIndex new_index = parent_index.child(row);
    convertIndexToInternalId(new_index,ref);

#ifdef DEBUG_MODEL_INDEX
    RsDbg() << "  returning " << createIndex(row,column,ref) << std::endl;
#endif

    return createIndex(row,column,ref);
}

QModelIndex RsIdentityListModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();

	EntryIndex I ;
    convertInternalIdToIndex(index.internalId(),I);

	EntryIndex p = I.parent();

    if(p.type == ENTRY_TYPE_TOP_LEVEL)
		return QModelIndex();

	quintptr i;
    convertIndexToInternalId(p,i);

    return createIndex(I.parentRow(),0,i);
}

Qt::ItemFlags RsIdentityListModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::ItemFlags();

	return QAbstractItemModel::flags(index);
}

QVariant RsIdentityListModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if(role == Qt::DisplayRole)
		switch(section)
		{
		case COLUMN_THREAD_NAME:         return tr("Name");
		case COLUMN_THREAD_ID:           return tr("Id");
        case COLUMN_THREAD_REPUTATION:   return tr("Reputation");
        case COLUMN_THREAD_OWNER:        return tr("Owner");
		default:
			return QVariant();
		}

	return QVariant();
}

QVariant RsIdentityListModel::data(const QModelIndex &index, int role) const
{
#ifdef DEBUG_MESSAGE_MODEL
    std::cerr << "calling data(" << index << ") role=" << role << std::endl;
#endif

	if(!index.isValid())
		return QVariant();

	quintptr ref = (index.isValid())?index.internalId():0 ;

#ifdef DEBUG_MESSAGE_MODEL
	std::cerr << "data(" << index << ")" ;
#endif

	if(!ref)
	{
#ifdef DEBUG_MESSAGE_MODEL
		std::cerr << " [empty]" << std::endl;
#endif
		return QVariant() ;
	}

	EntryIndex entry;

    if(!convertInternalIdToIndex(ref,entry))
	{
#ifdef DEBUG_MESSAGE_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	switch(role)
	{
	case Qt::SizeHintRole:   return sizeHintRole(entry,index.column()) ;
	case Qt::DisplayRole:    return displayRole(entry,index.column()) ;
	case Qt::FontRole:       return fontRole(entry,index.column()) ;
    case Qt::ForegroundRole: return foregroundRole(entry,index.column()) ;
    case Qt::DecorationRole: return decorationRole(entry,index.column()) ;

 	case FilterRole:         return filterRole(entry,index.column()) ;
 	case SortRole:           return sortRole(entry,index.column()) ;
    case TreePathRole:       return treePathRole(entry,index.column()) ;

	default:
		return QVariant();
	}
}

bool RsIdentityListModel::passesFilter(const EntryIndex& e,int /*column*/) const
{
	QString s ;
	bool passes_strings = true ;

    if(e.type == ENTRY_TYPE_IDENTITY && !mFilterStrings.empty())
	{
		switch(mFilterType)
		{
		case FILTER_TYPE_ID: 	s = displayRole(e,COLUMN_THREAD_ID).toString();
			break;

		case FILTER_TYPE_NAME:  s = displayRole(e,COLUMN_THREAD_NAME).toString();
			if(s.isNull())
				passes_strings = false;
			break;
		case FILTER_TYPE_NONE:
			RS_ERR("None Type for Filter.");
		};
	}

	if(!s.isNull())
		for(auto iter(mFilterStrings.begin()); iter != mFilterStrings.end(); ++iter)
			passes_strings = passes_strings && s.contains(*iter,Qt::CaseInsensitive);

	return passes_strings;
}

QVariant RsIdentityListModel::filterRole(const EntryIndex& e,int column) const
{
	if(passesFilter(e,column))
		return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsIdentityListModel::updateFilterStatus(ForumModelIndex /*i*/,int /*column*/,const QStringList& /*strings*/)
{
	return 0;
}


void RsIdentityListModel::setFilter(FilterType filter_type, const QStringList& strings)
{
#ifdef DEBUG_MODEL
    std::cerr << "Setting filter to filter_type=" << int(filter_type) << " and strings to " ;
    foreach(const QString& str,strings)
        std::cerr << "\"" << str.toStdString() << "\" " ;
    std::cerr << std::endl;
#endif

    preMods();

    mFilterType = filter_type;
	mFilterStrings = strings;

	postMods();
}

bool RsIdentityListModel::requestIdentityDetails(const RsGxsId& id,RsIdentityDetails& det) const
{
    if(!rsIdentity->getIdDetails(id,det))
    {
        mIdentityUpdateTimer->stop();
        mIdentityUpdateTimer->setSingleShot(true);
        mIdentityUpdateTimer->start(500);
        return false;
    }

    return true;
}
QVariant RsIdentityListModel::toolTipRole(const EntryIndex& fmpe,int /*column*/) const
{
    switch(fmpe.type)
    {
    case ENTRY_TYPE_IDENTITY:
    {
        RsGxsId id(mIdentities[mCategories[fmpe.category_index].child_identity_indices[fmpe.identity_index]].id);

        if(rsIdentity->isOwnId(id))
            return QVariant(tr("This identity is owned by you"));

        RsIdentityDetails det;
        if(!requestIdentityDetails(id,det))
            return QVariant();

        if(det.mPgpId.isNull())
            return QVariant("Anonymous identity");
        else
        {
            RsPeerDetails dd;
            rsPeers->getGPGDetails(det.mPgpId,dd);

            return QVariant("Identity owned by profile \""+ QString::fromUtf8(dd.name.c_str()) +"\" ("+QString::fromStdString(det.mPgpId.toStdString()));
        }
    }

        break;
    case ENTRY_TYPE_CATEGORY: ; // fallthrough
    default:
        return QVariant();
    }
}

QVariant RsIdentityListModel::sizeHintRole(const EntryIndex& e,int col) const
{
	float x_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;
	float y_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

    if(e.type == ENTRY_TYPE_IDENTITY)
        y_factor *= 1.0;

    if(e.type == ENTRY_TYPE_CATEGORY)
		y_factor *= 1.5;

	switch(col)
	{
	default:
    case COLUMN_THREAD_NAME:       return QVariant( QSize(x_factor * 70 , y_factor*14*1.1f ));
    case COLUMN_THREAD_ID:         return QVariant( QSize(x_factor * 175, y_factor*14*1.1f ));
    case COLUMN_THREAD_REPUTATION: return QVariant( QSize(x_factor * 20 , y_factor*14*1.1f ));
    case COLUMN_THREAD_OWNER:      return QVariant( QSize(x_factor * 70 , y_factor*14*1.1f ));
    }
}

QVariant RsIdentityListModel::treePathRole(const EntryIndex& entry,int column) const
{
    if(entry.type == ENTRY_TYPE_CATEGORY)
        return QString::number((int)entry.category_index);
    else
        return QString::fromStdString(mIdentities[entry.identity_index].id.toStdString());
}
QVariant RsIdentityListModel::sortRole(const EntryIndex& entry,int column) const
{
    switch(column)
    {
#warning TODO
//    case COLUMN_THREAD_LAST_CONTACT:
//    {
//        switch(entry.type)
//		{
//		case ENTRY_TYPE_PROFILE:
//		{
//			const HierarchicalProfileInformation *prof = getProfileInfo(entry);
//
//			if(!prof)
//				return QVariant();
//
//            uint32_t last_contact = 0;
//
//			for(uint32_t i=0;i<prof->child_node_indices.size();++i)
//                last_contact = std::max(last_contact, mLocations[prof->child_node_indices[i]].node_info.lastConnect);
//
//            return QVariant(last_contact);
//		}
//            break;
//        default:
//            return QVariant();
//		}
//    }
//        break;

    default:
		return displayRole(entry,column);
    }
}

QModelIndex RsIdentityListModel::getIndexOfIdentity(const RsGxsId& id) const
{
    for(uint i=0;i<mCategories.size();++i)
        for(uint j=0;j<mCategories[i].child_identity_indices.size();++j)
            if(mIdentities[mCategories[i].child_identity_indices[j]].id == id)
            {
                EntryIndex e;
                e.category_index = i;
                e.identity_index = j;
                e.type = ENTRY_TYPE_IDENTITY;
                quintptr idx;
                convertIndexToInternalId(e,idx);

                return createIndex(j,0,idx);
            }
    return QModelIndex();
}
QVariant RsIdentityListModel::foregroundRole(const EntryIndex& e, int /*col*/) const
{
    auto it = getIdentityInfo(e);
    if(!it)
        return QVariant();
    RsGxsId id(it->id);
    RsIdentityDetails det;

    if(!requestIdentityDetails(id,det))
        return QVariant();

    if(det.mFlags & RS_IDENTITY_FLAGS_IS_DEPRECATED)
        return QVariant(QColor(Qt::red));

    return QVariant();
}
QVariant RsIdentityListModel::fontRole(const EntryIndex& e, int /*col*/) const
{
        auto it = getIdentityInfo(e);
    if(!it)
        return QVariant();
    RsGxsId id(it->id);

    if(rsIdentity->isOwnId(id))
    {
        QFont f;
        f.setBold(true);
        return QVariant(f);
    }
    else
        return QVariant();
}

#ifdef DEBUG_MODEL_INDEX
	std::cerr << "  font role " << e.type << ", (" << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": " << std::endl;
#endif

#ifdef TODO
	int status = onlineRole(e,col).toInt();

	switch (status)
	{
		case RS_STATUS_INACTIVE:
		{
			QFont font ;
			QTreeView* myParent = dynamic_cast<QTreeView*>(QAbstractItemModel::parent());
			if (myParent)
				font = myParent->font();

			font.setBold(true);

			return QVariant(font);
		}
		default:
		return QVariant();
	}
#endif

QVariant RsIdentityListModel::displayRole(const EntryIndex& e, int col) const
{
#ifdef DEBUG_MODEL_INDEX
	std::cerr << "  Display role " << e.type << ", (" << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": ";
#endif

	switch(e.type)
	{
        case ENTRY_TYPE_CATEGORY:
		{
            const HierarchicalCategoryInformation *cat = getCategoryInfo(e);

            if(!cat)
				return QVariant();

			switch(col)
			{
				case COLUMN_THREAD_NAME:
#ifdef DEBUG_MODEL_INDEX
					std::cerr <<   group->group_info.name.c_str() ;
#endif

                    if(!cat->child_identity_indices.empty())
                        return QVariant(cat->category_name+" (" + QString::number(cat->child_identity_indices.size()) + ")");
					else
                        return QVariant(cat->category_name);

				default:
				return QVariant();
			}
		}
		break;

        case ENTRY_TYPE_IDENTITY:
        {
                const HierarchicalIdentityInformation *idinfo = getIdentityInfo(e);

                if(!idinfo)
                        return QVariant();

                RsIdentityDetails det;

                if(!requestIdentityDetails(idinfo->id,det))
                    return QVariant();

#ifdef DEBUG_MODEL_INDEX
                std::cerr << profile->profile_info.name.c_str() ;
#endif
                switch(col)
                {
                case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(det.mNickname.c_str()));
                case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(det.mId.toStdString()) );
                case COLUMN_THREAD_OWNER:          return QVariant(QString::fromStdString(det.mPgpId.toStdString()) );
                default:
                        return QVariant();
                }
        }
		break;

		default: //ENTRY_TYPE
		return QVariant();
	}
}

// This function makes sure that the internal data gets updated. They are situations where the notification system cannot
// send the information about changes, such as when the computer is put on sleep.

void RsIdentityListModel::checkInternalData(bool force)
{
	rstime_t now = time(NULL);

    if( (mLastInternalDataUpdate + MAX_INTERNAL_DATA_UPDATE_DELAY < now) || force)
        updateIdentityList();
}

const RsIdentityListModel::HierarchicalCategoryInformation *RsIdentityListModel::getCategoryInfo(const EntryIndex& e) const
{
    if(e.category_index >= mCategories.size())
        return NULL ;
    else
        return &mCategories[e.category_index];
}

const RsIdentityListModel::HierarchicalIdentityInformation *RsIdentityListModel::getIdentityInfo(const EntryIndex& e) const
{
    // First look into the relevant group, then for the correct profile in this group.

    if(e.type != ENTRY_TYPE_IDENTITY)
        return NULL ;

    if(e.category_index >= mCategories.size())
        return NULL ;

    if(e.identity_index < mCategories[e.category_index].child_identity_indices.size())
        return &mIdentities[mCategories[e.category_index].child_identity_indices[e.identity_index]];
    else
        return &mIdentities[e.identity_index];
}

QVariant RsIdentityListModel::decorationRole(const EntryIndex& entry,int col) const
{
    switch(entry.type)
    {
    case ENTRY_TYPE_CATEGORY:
        return QVariant();

    case ENTRY_TYPE_IDENTITY:
    {
        const HierarchicalIdentityInformation *hn = getIdentityInfo(entry);

        if(!hn)
            return QVariant();

        if(col == COLUMN_THREAD_REPUTATION)
            return QVariant( static_cast<uint8_t>(rsReputations->overallReputationLevel(hn->id)) );
        else if(col == COLUMN_THREAD_NAME)
        {
            QPixmap sslAvatar;
            AvatarDefs::getAvatarFromGxsId(hn->id, sslAvatar);

            return QVariant(QIcon(sslAvatar));
        }
        else
            return QVariant();
    }
        break;

    default:
        return QVariant();
    }
}

void RsIdentityListModel::clear()
{
    preMods();

    mIdentities.clear();
    mCategories.clear();

    mCategories.resize(3);
    mCategories[0].category_name = tr("My own identities");
    mCategories[1].category_name = tr("My contacts");
    mCategories[2].category_name = tr("All");

	postMods();

	emit friendListChanged();
}

void RsIdentityListModel::debug_dump() const
{
    std::cerr << "==== IdentityListModel Debug dump ====" << std::endl;

    std::cerr << "Invalid index  : " << QModelIndex() << std::endl;

    EntryIndex top_level;
    top_level.type = ENTRY_TYPE_TOP_LEVEL;
    QModelIndex created_top_level;
    quintptr id;
    convertIndexToInternalId(top_level,id);

    std::cerr << "Top level index: " << createIndex(0,0,id) << std::endl;

    for(uint32_t j=0;j<mCategories.size();++j)
    {
        std::cerr << mCategories[j].category_name.toStdString() << " (" << mCategories[j].child_identity_indices.size() << ")" << std::endl;
        const auto& hg(mCategories[j].child_identity_indices);

        for(uint32_t i=0;i<hg.size();++i)
        {
            auto idx = getIndexOfIdentity(mIdentities[hg[i]].id);
            auto parent = idx.parent();
            EntryIndex index;
            EntryIndex index_parent;
            convertInternalIdToIndex(idx.internalId(),index);
            convertInternalIdToIndex(parent.internalId(),index_parent);
            std::cerr << "       " << mIdentities[hg[i]].id << " index = " << idx << " parent = " << idx.parent() << " EntryIndex: " << index << " Parent index: " << index_parent << std::endl;
        }
    }

    std::cerr << "====================================" << std::endl;
}


RsGxsId RsIdentityListModel::getIdentity(const QModelIndex& i) const
{
    if(!i.isValid())
        return RsGxsId();

    EntryIndex e;
    if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_IDENTITY)
        return RsGxsId();

    const HierarchicalIdentityInformation *gnode = getIdentityInfo(e);

    if(gnode)
        return gnode->id;
    else
        return RsGxsId();
}

RsIdentityListModel::EntryType RsIdentityListModel::getType(const QModelIndex& i) const
{
	if(!i.isValid())
        return ENTRY_TYPE_TOP_LEVEL;

	EntryIndex e;
    if(!convertInternalIdToIndex(i.internalId(),e))
        return ENTRY_TYPE_TOP_LEVEL;

    return e.type;
}

int RsIdentityListModel::getCategory(const QModelIndex& i) const
{
    if(!i.isValid())
        return CATEGORY_ALL;

    EntryIndex e;
    if(!convertInternalIdToIndex(i.internalId(),e))
        return CATEGORY_ALL;

    return e.category_index;
}
void RsIdentityListModel::setIdentities(const std::list<RsGroupMetaData>& identities_meta)
{
    preMods();
    beginResetModel();
    clear();

    for(auto id:identities_meta)
    {
        HierarchicalIdentityInformation idinfo;
        idinfo.id = RsGxsId(id.mGroupId);

        if(rsIdentity->isOwnId(idinfo.id))
            mCategories[CATEGORY_OWN].child_identity_indices.push_back(mIdentities.size());
        else if(rsIdentity->isARegularContact(RsGxsId(id.mGroupId)))
            mCategories[CATEGORY_CTS].child_identity_indices.push_back(mIdentities.size());
        else
            mCategories[CATEGORY_ALL].child_identity_indices.push_back(mIdentities.size());

        mIdentities.push_back(idinfo);
    }

    if (mCategories.size()>0)
    {
        beginInsertRows(QModelIndex(),0,mCategories.size()-1);
        endInsertRows();
    }

    endResetModel();
    postMods();

    mLastInternalDataUpdate = time(NULL);

}

void RsIdentityListModel::updateIdentityList()
{
    std::cerr << "Updating identity list" << std::endl;

    std::list<RsGroupMetaData> ids ;

    if(!rsIdentity->getIdentitiesSummaries(ids))
    {
        std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve identity metadata." << std::endl;
        return;
    }

    setIdentities(ids) ;
}



void RsIdentityListModel::collapseItem(const QModelIndex& index)
{
    if(getType(index) != ENTRY_TYPE_CATEGORY)
        return;

    EntryIndex entry;

    if(!convertInternalIdToIndex(index.internalId(),entry))
        return;

    mExpandedCategories[entry.category_index] = false;

    // apparently we cannot be subtle here.
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mCategories.size()-1,columnCount()-1,(void*)NULL));
}

void RsIdentityListModel::expandItem(const QModelIndex& index)
{
    if(getType(index) != ENTRY_TYPE_CATEGORY)
        return;

    EntryIndex entry;

    if(!convertInternalIdToIndex(index.internalId(),entry))
        return;

    mExpandedCategories[entry.category_index] = true;

    // apparently we cannot be subtle here.
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mCategories.size()-1,columnCount()-1,(void*)NULL));
}

bool RsIdentityListModel::isCategoryExpanded(const EntryIndex& e) const
{
    return true;
#warning TODO
//     if(e.type != ENTRY_TYPE_CATEGORY)
//         return false;
//
//     EntryIndex entry;
//
//     if(!convertInternalIdToIndex<sizeof(uintptr_t)>(e.internalId(),entry))
//         return false;
//
//     return mExpandedCategories[entry.category_index];
}

