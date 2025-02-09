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

static const uint16_t UNDEFINED_GROUP_INDEX_VALUE   = (sizeof(uintptr_t)==4)?0x1ff:0xffff;	// max value for 9 bits
static const uint16_t UNDEFINED_NODE_INDEX_VALUE    = (sizeof(uintptr_t)==4)?0x1ff:0xffff;  // max value for 9 bits
static const uint16_t UNDEFINED_PROFILE_INDEX_VALUE = (sizeof(uintptr_t)==4)?0xfff:0xffff;  // max value for 12 bits

const QString RsIdentityListModel::FilterString("filtered");

const uint32_t MAX_INTERNAL_DATA_UPDATE_DELAY = 300 ; 	// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.
const uint32_t MAX_NODE_UPDATE_DELAY = 10 ; 			// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.

static const uint32_t NODE_DETAILS_UPDATE_DELAY = 5;	// update each node every 5 secs.

RsIdentityListModel::RsIdentityListModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mLastInternalDataUpdate(0), mLastNodeUpdate(0)
{
	mFilterStrings.clear();
}

RsIdentityListModel::EntryIndex::EntryIndex()
   : type(ENTRY_TYPE_UNKNOWN),category_index(UNDEFINED_GROUP_INDEX_VALUE),identity_index(UNDEFINED_NODE_INDEX_VALUE)
{
}

// The index encodes the whole hierarchy of parents. This allows to very efficiently compute indices of the parent of an index.
//
// On 32 bits and 64 bits architectures the format is the following:
//
//     0x [2 bits] [30 bits]
//            |        |
//            |        |
//            |        |
//            |        +----------------------- identity index
//            +-------------------------------- category
//
// Only valid indexes a 0x00->UNDEFINED_INDEX_VALUE-1.

bool RsIdentityListModel::convertIndexToInternalId(const EntryIndex& e,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    id = (((uint32_t)e.type) << 30) + ((uint32_t)e.identity_index);
	return true;
}
bool RsIdentityListModel::convertInternalIdToIndex(quintptr ref,EntryIndex& e)
{
    if(ref == 0)
        return false ;

    e.category_index  = (ref >> 30) & 0x3;// 2 bits
    e.identity_index  = (ref >>  0) & 0x3fffffff;// 30 bits

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

	if(parent.internalId() == 0)
		return mTopLevel.size();

	EntryIndex index;
    if(!convertInternalIdToIndex(parent.internalId(),index))
		return 0;

    if(index.type == ENTRY_TYPE_CATEGORY)
        return mCategories[index.category_index].child_identity_indices.size();
    else
		return 0;
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
        case ENTRY_TYPE_CATEGORY: return EntryIndex();

        case ENTRY_TYPE_IDENTITY: i.type = ENTRY_TYPE_CATEGORY;
            i.identity_index = UNDEFINED_NODE_INDEX_VALUE;
		break;
		case ENTRY_TYPE_UNKNOWN:
			//Can be when request root index.
		break;
	}

	return i;
}

RsIdentityListModel::EntryIndex RsIdentityListModel::EntryIndex::child(int row,const std::vector<EntryIndex>& top_level) const
{
    EntryIndex i(*this);

	switch(type)
    {
    case ENTRY_TYPE_UNKNOWN:
						   i = top_level[row];
						   break;

    case ENTRY_TYPE_CATEGORY: i.type = ENTRY_TYPE_IDENTITY;
                           i.identity_index = row;
						   break;

    case ENTRY_TYPE_IDENTITY:  i = EntryIndex();
						   break;
    }

    return i;

}
uint32_t   RsIdentityListModel::EntryIndex::parentRow(int /* nb_groups */) const
{
    switch(type)
    {
    default:
    	case ENTRY_TYPE_UNKNOWN  : return 0;
        case ENTRY_TYPE_CATEGORY : return category_index;
        case ENTRY_TYPE_IDENTITY : return identity_index;
    }
}

QModelIndex RsIdentityListModel::index(int row, int column, const QModelIndex& parent) const
{
    if(row < 0 || column < 0 || column >= columnCount(parent) || row >= rowCount(parent))
		return QModelIndex();

    if(parent.internalId() == 0)
    {
		quintptr ref ;

        convertIndexToInternalId(mTopLevel[row],ref);
		return createIndex(row,column,ref) ;
    }

    EntryIndex parent_index ;
    convertInternalIdToIndex(parent.internalId(),parent_index);
#ifdef DEBUG_MODEL_INDEX
    RsDbg() << "Index row=" << row << " col=" << column << " parent=" << parent << std::endl;
#endif

    quintptr ref;
    EntryIndex new_index = parent_index.child(row,mTopLevel);
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

	if(p.type == ENTRY_TYPE_UNKNOWN)
		return QModelIndex();

	quintptr i;
    convertIndexToInternalId(p,i);

    return createIndex(I.parentRow(mCategories.size()),0,i);
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
 	case Qt::DecorationRole: return decorationRole(entry,index.column()) ;

 	case FilterRole:         return filterRole(entry,index.column()) ;
 	case SortRole:           return sortRole(entry,index.column()) ;

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

QVariant RsIdentityListModel::toolTipRole(const EntryIndex& /*fmpe*/,int /*column*/) const
{
    return QVariant();
}

QVariant RsIdentityListModel::sizeHintRole(const EntryIndex& e,int col) const
{
	float x_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;
	float y_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

    if(e.type == ENTRY_TYPE_IDENTITY)
		y_factor *= 3.0;

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

QVariant RsIdentityListModel::fontRole(const EntryIndex& e, int col) const
{
#ifdef DEBUG_MODEL_INDEX
	std::cerr << "  font role " << e.type << ", (" << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": " << std::endl;
#endif

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
}

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

                if(!rsIdentity->getIdDetails(idinfo->id,det))
                    return QVariant();

#ifdef DEBUG_MODEL_INDEX
                std::cerr << profile->profile_info.name.c_str() ;
#endif
                switch(col)
                {
                case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(det.mNickname.c_str()));
                case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(det.mId.toStdString()) );
                case COLUMN_THREAD_OWNER:          return QVariant(QString::fromStdString(det.mPgpId.toStdString()) );
                case COLUMN_THREAD_REPUTATION:     return QVariant(QString::number((uint8_t)det.mReputation.mOverallReputationLevel));
                default:
                        return QVariant();
                }
        }
		break;

		default: //ENTRY_TYPE
		return QVariant();
	}
}

// This function makes sure that the internal data gets updated. They are situations where the otification system cannot
// send the information about changes, such as when the computer is put on sleep.

void RsIdentityListModel::checkInternalData(bool force)
{
	rstime_t now = time(NULL);

    if( (mLastInternalDataUpdate + MAX_INTERNAL_DATA_UPDATE_DELAY < now) || force)
		updateInternalData();
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
    if(col > 0)
        return QVariant();

    switch(entry.type)
    {

    case ENTRY_TYPE_CATEGORY:
        return QVariant();

    case ENTRY_TYPE_IDENTITY:
    {
        const HierarchicalIdentityInformation *hn = getIdentityInfo(entry);

        if(!hn)
            return QVariant();

		QPixmap sslAvatar;
        AvatarDefs::getAvatarFromGxsId(hn->id, sslAvatar);

        return QVariant(QIcon(sslAvatar));
    }
    default: return QVariant();
    }
}

void RsIdentityListModel::clear()
{
    preMods();

    mIdentities.clear();
    mTopLevel.clear();

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
//    std::cerr << "==== FriendListModel Debug dump ====" << std::endl;
//
//	for(uint32_t j=0;j<mTopLevel.size();++j)
//    {
//        if(mTopLevel[j].type == ENTRY_TYPE_GROUP)
//		{
//			const HierarchicalGroupInformation& hg(mGroups[mTopLevel[j].group_index]);
//
//			std::cerr << "Group: " << hg.group_info.name << ", ";
//			std::cerr << "  children indices: " ; for(uint32_t i=0;i<hg.child_profile_indices.size();++i) std::cerr << hg.child_profile_indices[i] << " " ; std::cerr << std::endl;
//
//			for(uint32_t i=0;i<hg.child_profile_indices.size();++i)
//			{
//				uint32_t profile_index = hg.child_profile_indices[i];
//
//				std::cerr << "    Profile " << mProfiles[profile_index].profile_info.gpg_id << std::endl;
//
//				const HierarchicalProfileInformation& hprof(mProfiles[profile_index]);
//
//				for(uint32_t k=0;k<hprof.child_node_indices.size();++k)
//					std::cerr << "      Node " << mLocations[hprof.child_node_indices[k]].node_info.id << std::endl;
//			}
//		}
//        else if(mTopLevel[j].type == ENTRY_TYPE_PROFILE)
//        {
//			const HierarchicalProfileInformation& hprof(mProfiles[mTopLevel[j].profile_index]);
//
//			std::cerr << "Profile " << hprof.profile_info.gpg_id << std::endl;
//
//			for(uint32_t k=0;k<hprof.child_node_indices.size();++k)
//				std::cerr << "  Node " << mLocations[hprof.child_node_indices[k]].node_info.id << std::endl;
//        }
//    }
//    std::cerr << "====================================" << std::endl;
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
		return ENTRY_TYPE_UNKNOWN;

	EntryIndex e;
    if(!convertInternalIdToIndex(i.internalId(),e))
        return ENTRY_TYPE_UNKNOWN;

    return e.type;
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

void RsIdentityListModel::updateInternalData()
{
    RsThread::async([this]()
    {
        // 1 - get message data from p3GxsForums

        std::list<RsGroupMetaData> *ids = new std::list<RsGroupMetaData>();

        if(!rsIdentity->getIdentitiesSummaries(*ids))
        {
            std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve identity metadata." << std::endl;
            return;
        }

        // 3 - update the model in the UI thread.

        RsQThreadUtils::postToObject( [ids,this]()
        {
            /* Here it goes any code you want to be executed on the Qt Gui
             * thread, for example to update the data model with new information
             * after a blocking call to RetroShare API complete, note that
             * Qt::QueuedConnection is important!
             */

            setIdentities(*ids) ;

            delete ids;


        }, this );

    });

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
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mTopLevel.size()-1,columnCount()-1,(void*)NULL));
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
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mTopLevel.size()-1,columnCount()-1,(void*)NULL));
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

