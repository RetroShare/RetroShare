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
#include <QIcon>

#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/AvatarDefs.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/common/FriendListModel.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "retroshare/rsexpr.h"
#include "retroshare/rsmsgs.h"

//#define DEBUG_MODEL
//#define DEBUG_MODEL_INDEX

#define IS_MESSAGE_UNREAD(flags) (flags &  (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER))

#define IMAGE_COWORKERS        ":/icons/groups/green.svg"
#define IMAGE_FRIENDS          ":/icons/groups/blue.svg"
#define IMAGE_FAMILY           ":/icons/groups/purple.svg"
#define IMAGE_FAVORITES        ":/icons/groups/yellow.svg"
#define IMAGE_OTHERCONTACTS    ":/icons/groups/pink.svg"
#define IMAGE_OTHERGROUPS      ":/icons/groups/red.svg"
#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

static const uint16_t UNDEFINED_GROUP_INDEX_VALUE   = (sizeof(uintptr_t)==4)?0x1ff:0xffff;	// max value for 9 bits
static const uint16_t UNDEFINED_NODE_INDEX_VALUE    = (sizeof(uintptr_t)==4)?0x1ff:0xffff;  // max value for 9 bits
static const uint16_t UNDEFINED_PROFILE_INDEX_VALUE = (sizeof(uintptr_t)==4)?0xfff:0xffff;  // max value for 12 bits

const QString RsFriendListModel::FilterString("filtered");
const uint32_t MAX_INTERNAL_DATA_UPDATE_DELAY = 300 ; 	// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.
const uint32_t MAX_NODE_UPDATE_DELAY = 10 ; 			// re-update the internal data every 5 mins. Should properly cover sleep/wake-up changes.

static const uint32_t NODE_DETAILS_UPDATE_DELAY = 5;	// update each node every 5 secs.

RsFriendListModel::RsFriendListModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mDisplayGroups(true), mDisplayStatusString(true)
    , mLastInternalDataUpdate(0), mLastNodeUpdate(0)
{
	mFilterStrings.clear();
}

RsFriendListModel::EntryIndex::EntryIndex()
   : type(ENTRY_TYPE_UNKNOWN),group_index(UNDEFINED_GROUP_INDEX_VALUE),profile_index(UNDEFINED_PROFILE_INDEX_VALUE),node_index(UNDEFINED_NODE_INDEX_VALUE)
{
}

// The index encodes the whole hierarchy of parents. This allows to very efficiently compute indices of the parent of an index.
//
// On 32 bits architectures the format is the following:
//
//     0x [2 bits] [9 bits] [12 bits] [9 bits]
//            |        |        |         |
//            |        |        |         +---- location/node index
//            |        |        +-------------- profile index
//            |        +----------------------- group index
//            +-------------------------------- type
//
// On 64 bits architectures the format is the following:
//
//     0x [16 bits] [16 bits] [16 bits] [16 bits]
//            |        |        |         |
//            |        |        |         +---- location/node index
//            |        |        +-------------- profile index
//            |        +----------------------- group index
//            +-------------------------------- type
//
// Only valid indexes a 0x00->UNDEFINED_INDEX_VALUE-1.

template<> bool RsFriendListModel::convertIndexToInternalId<4>(const EntryIndex& e,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    id = (((uint32_t)e.type) << 30) + ((uint32_t)e.group_index << 21) + ((uint32_t)e.profile_index << 9) + (uint32_t)e.node_index;
	return true;
}
template<> bool RsFriendListModel::convertIndexToInternalId<8>(const EntryIndex& e,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    id = (((uint64_t)e.type) << 48) + ((uint64_t)e.group_index << 32) + ((uint64_t)e.profile_index << 16) + (uint64_t)e.node_index;
	return true;
}

template<> bool RsFriendListModel::convertInternalIdToIndex<4>(quintptr ref,EntryIndex& e)
{
    if(ref == 0)
        return false ;

    e.group_index     = (ref >> 21) & 0x1ff;// 9 bits
    e.profile_index   = (ref >>  9) & 0xfff;// 12 bits
    e.node_index      = (ref >>  0) & 0x1ff;// 9 bits

    e.type = static_cast<EntryType>((ref >> 30) & 0x03);

	return true;
}

template<> bool RsFriendListModel::convertInternalIdToIndex<8>(quintptr ref,EntryIndex& e)
{
    if(ref == 0)
        return false ;

    e.group_index     = (ref >> 32) & 0xffff;
    e.profile_index   = (ref >> 16) & 0xffff;
    e.node_index      = (ref >>  0) & 0xffff;

    e.type = static_cast<EntryType>((ref >> 48) & 0xffff);

	return true;
}


void RsFriendListModel::setDisplayStatusString(bool b)
{
    mDisplayStatusString = b;
	postMods();
}

void RsFriendListModel::setDisplayGroups(bool b)
{
    mDisplayGroups = b;

    updateInternalData();
}
void RsFriendListModel::preMods()
{
	emit layoutAboutToBeChanged();
}
void RsFriendListModel::postMods()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(rowCount()-1,columnCount()-1,(void*)NULL));
	emit layoutChanged();
}

int RsFriendListModel::rowCount(const QModelIndex& parent) const
{
	if(parent.column() >= COLUMN_THREAD_NB_COLUMNS)
		return 0;

	if(parent.internalId() == 0)
		return mTopLevel.size();

	EntryIndex index;
	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(parent.internalId(),index))
		return 0;

	if(index.type == ENTRY_TYPE_GROUP)
		return mGroups[index.group_index].child_profile_indices.size();

	if(index.type == ENTRY_TYPE_PROFILE)
	{
		if(index.group_index < UNDEFINED_GROUP_INDEX_VALUE)
			return mProfiles[mGroups[index.group_index].child_profile_indices[index.profile_index]].child_node_indices.size();
		else
			return mProfiles[index.profile_index].child_node_indices.size();
	}
	else //if(index.type == ENTRY_TYPE_NODE)
		return 0;
}

int RsFriendListModel::columnCount(const QModelIndex &/*parent*/) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

bool RsFriendListModel::hasChildren(const QModelIndex &parent) const
{
	if(!parent.isValid())
		return true;

	EntryIndex parent_index ;
	convertInternalIdToIndex<sizeof(uintptr_t)>(parent.internalId(),parent_index);

	if(parent_index.type == ENTRY_TYPE_NODE)
		return false;

	if(parent_index.type == ENTRY_TYPE_PROFILE)
	{
		if(parent_index.group_index < UNDEFINED_GROUP_INDEX_VALUE)
			return !mProfiles[mGroups[parent_index.group_index].child_profile_indices[parent_index.profile_index]].child_node_indices.empty();
		else
			return !mProfiles[parent_index.profile_index].child_node_indices.empty();
	}
	if(parent_index.type == ENTRY_TYPE_GROUP)
		return !mGroups[parent_index.group_index].child_profile_indices.empty();

	return false;
}

RsFriendListModel::EntryIndex RsFriendListModel::EntryIndex::parent() const
{
	EntryIndex i(*this);

	switch(type)
	{
		case ENTRY_TYPE_GROUP: return EntryIndex();

		case ENTRY_TYPE_PROFILE:
			if(i.group_index==UNDEFINED_GROUP_INDEX_VALUE)
				return EntryIndex();
			else
			{
				i.type = ENTRY_TYPE_GROUP;
				i.profile_index = UNDEFINED_PROFILE_INDEX_VALUE;
			}
		break;

		case ENTRY_TYPE_NODE:  i.type = ENTRY_TYPE_PROFILE;
			i.node_index = UNDEFINED_NODE_INDEX_VALUE;
		break;
		case ENTRY_TYPE_UNKNOWN:
			//Can be when request root index.
		break;
	}

	return i;
}

RsFriendListModel::EntryIndex RsFriendListModel::EntryIndex::child(int row,const std::vector<EntryIndex>& top_level) const
{
    EntryIndex i(*this);

	switch(type)
    {
    case ENTRY_TYPE_UNKNOWN:
						   i = top_level[row];
						   break;

    case ENTRY_TYPE_GROUP: i.type = ENTRY_TYPE_PROFILE;
        				   i.profile_index = row;
						   break;

    case ENTRY_TYPE_PROFILE: i.type = ENTRY_TYPE_NODE;
        				   i.node_index = row;
						   break;

    case ENTRY_TYPE_NODE:  i = EntryIndex();
						   break;
    }

    return i;

}
uint32_t   RsFriendListModel::EntryIndex::parentRow(uint32_t nb_groups) const
{
    switch(type)
    {
    default:
    	case ENTRY_TYPE_UNKNOWN  : return 0;
    	case ENTRY_TYPE_GROUP    : return group_index;
		case ENTRY_TYPE_PROFILE  : return (group_index==UNDEFINED_GROUP_INDEX_VALUE)?(profile_index+nb_groups):profile_index;
    	case ENTRY_TYPE_NODE     : return node_index;
    }
}

QModelIndex RsFriendListModel::index(int row, int column, const QModelIndex& parent) const
{
    if(row < 0 || column < 0 || column >= columnCount(parent) || row >= rowCount(parent))
		return QModelIndex();

    if(parent.internalId() == 0)
    {
		quintptr ref ;

		convertIndexToInternalId<sizeof(uintptr_t)>(mTopLevel[row],ref);
		return createIndex(row,column,ref) ;
    }

    EntryIndex parent_index ;
    convertInternalIdToIndex<sizeof(uintptr_t)>(parent.internalId(),parent_index);
#ifdef DEBUG_MODEL_INDEX
    RsDbg() << "Index row=" << row << " col=" << column << " parent=" << parent << std::endl;
#endif

    quintptr ref;
    EntryIndex new_index = parent_index.child(row,mTopLevel);
    convertIndexToInternalId<sizeof(uintptr_t)>(new_index,ref);

#ifdef DEBUG_MODEL_INDEX
    RsDbg() << "  returning " << createIndex(row,column,ref) << std::endl;
#endif

    return createIndex(row,column,ref);
}

QModelIndex RsFriendListModel::parent(const QModelIndex& index) const
{
	if(!index.isValid())
		return QModelIndex();

	EntryIndex I ;
	convertInternalIdToIndex<sizeof(uintptr_t)>(index.internalId(),I);

	EntryIndex p = I.parent();

	if(p.type == ENTRY_TYPE_UNKNOWN)
		return QModelIndex();

	quintptr i;
	convertIndexToInternalId<sizeof(uintptr_t)>(p,i);

	return createIndex(I.parentRow(mGroups.size()),0,i);
}

Qt::ItemFlags RsFriendListModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
		return Qt::ItemFlags();

	return QAbstractItemModel::flags(index);
}

QVariant RsFriendListModel::headerData(int section, Qt::Orientation /*orientation*/, int role) const
{
	if(role == Qt::DisplayRole)
		switch(section)
		{
		case COLUMN_THREAD_NAME:         return tr("Name");
		case COLUMN_THREAD_ID:           return tr("Id");
		case COLUMN_THREAD_LAST_CONTACT: return tr("Last contact");
		case COLUMN_THREAD_IP:           return tr("IP");
		default:
			return QVariant();
		}

	return QVariant();
}

QVariant RsFriendListModel::data(const QModelIndex &index, int role) const
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

	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(ref,entry))
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
 	case Qt::TextColorRole:  return textColorRole(entry,index.column()) ;
 	case Qt::DecorationRole: return decorationRole(entry,index.column()) ;

 	case FilterRole:         return filterRole(entry,index.column()) ;
 	case SortRole:           return sortRole(entry,index.column()) ;
 	case OnlineRole:         return onlineRole(entry,index.column()) ;
 	case TypeRole:           return QVariant((int)entry.type);

	default:
		return QVariant();
	}
}

QVariant RsFriendListModel::textColorRole(const EntryIndex& fmpe,int column) const
{
	switch(fmpe.type)
	{
		case ENTRY_TYPE_GROUP: return QVariant(QBrush(mTextColorGroup));
		case ENTRY_TYPE_PROFILE:
		case ENTRY_TYPE_NODE:  return QVariant(QBrush(mTextColorStatus[onlineRole(fmpe,column).toInt()]));
		default:
		return QVariant();
	}
}

QVariant RsFriendListModel::statusRole(const EntryIndex& /*fmpe*/,int /*column*/) const
{
    return QVariant();//fmpe.mMsgStatus);
}

bool RsFriendListModel::passesFilter(const EntryIndex& e,int /*column*/) const
{
	QString s ;
	bool passes_strings = true ;

	if(e.type == ENTRY_TYPE_PROFILE && !mFilterStrings.empty())
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

QVariant RsFriendListModel::filterRole(const EntryIndex& e,int column) const
{
	if(passesFilter(e,column))
		return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsFriendListModel::updateFilterStatus(ForumModelIndex /*i*/,int /*column*/,const QStringList& /*strings*/)
{
	return 0;
}


void RsFriendListModel::setFilter(FilterType filter_type, const QStringList& strings)
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

QVariant RsFriendListModel::toolTipRole(const EntryIndex& /*fmpe*/,int /*column*/) const
{
    return QVariant();
}

QVariant RsFriendListModel::sizeHintRole(const EntryIndex& e,int col) const
{
	float x_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;
	float y_factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	if(e.type == ENTRY_TYPE_NODE)
		y_factor *= 3.0;

	if((e.type == ENTRY_TYPE_PROFILE) && !isProfileExpanded(e))
		y_factor *= 3.0;

	if(e.type == ENTRY_TYPE_GROUP)
		y_factor *= 1.5;

	switch(col)
	{
	default:
	case COLUMN_THREAD_NAME:               return QVariant( QSize(x_factor * 170, y_factor*14*1.1f ));
	case COLUMN_THREAD_IP:                 return QVariant( QSize(x_factor * 75 , y_factor*14*1.1f ));
	case COLUMN_THREAD_ID:                 return QVariant( QSize(x_factor * 75 , y_factor*14*1.1f ));
	case COLUMN_THREAD_LAST_CONTACT:       return QVariant( QSize(x_factor * 75 , y_factor*14*1.1f ));
	}
}

QVariant RsFriendListModel::sortRole(const EntryIndex& entry,int column) const
{
    switch(column)
    {
    case COLUMN_THREAD_LAST_CONTACT:
    {
        switch(entry.type)
		{
		case ENTRY_TYPE_PROFILE:
		{
			const HierarchicalProfileInformation *prof = getProfileInfo(entry);

			if(!prof)
				return QVariant();

            uint32_t last_contact = 0;

			for(uint32_t i=0;i<prof->child_node_indices.size();++i)
                last_contact = std::max(last_contact, mLocations[prof->child_node_indices[i]].node_info.lastConnect);

            return QVariant(last_contact);
		}
            break;
        default:
            return QVariant();
		}
    }
        break;

    default:
		return displayRole(entry,column);
    }
}

QVariant RsFriendListModel::onlineRole(const EntryIndex& e, int /*col*/) const
{
	switch(e.type)
	{
		default:
		case ENTRY_TYPE_GROUP:
		{
			const HierarchicalGroupInformation& g(mGroups[e.group_index]);

			for(uint32_t j=0;j<g.child_profile_indices.size();++j)
			{
				const HierarchicalProfileInformation& prof = mProfiles[g.child_profile_indices[j]];

				for(uint32_t i=0;i<prof.child_node_indices.size();++i)
					if(mLocations[prof.child_node_indices[i]].node_info.state & RS_PEER_STATE_CONNECTED)
						return QVariant(RS_STATUS_ONLINE);
			}
			break;
		}

		case ENTRY_TYPE_PROFILE:
		{
			const HierarchicalProfileInformation *prof = getProfileInfo(e);

			if(prof)
			{
				for(uint32_t i=0;i<prof->child_node_indices.size();++i)
					if(mLocations[prof->child_node_indices[i]].node_info.state & RS_PEER_STATE_CONNECTED)
						return QVariant(RS_STATUS_ONLINE);
			}
		}
		break;

		case ENTRY_TYPE_NODE:
		{
			const HierarchicalNodeInformation *node = getNodeInfo(e);

			if(node)
			{
				StatusInfo status;
				rsStatus->getStatus(node->node_info.id, status);

				return QVariant(status.status);
			}
		}
	}
	return QVariant(RS_STATUS_OFFLINE);
}

QVariant RsFriendListModel::fontRole(const EntryIndex& e, int col) const
{
#ifdef DEBUG_MODEL_INDEX
	std::cerr << "  font role " << e.type << ", (" << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": " << std::endl;
#endif

	int status = onlineRole(e,col).toInt();

	switch (status)
	{
		case RS_STATUS_AWAY:
		case RS_STATUS_BUSY:
		case RS_STATUS_ONLINE:
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

class AutoEndel
{
public:
    ~AutoEndel() { std::cerr << std::endl;}
};

QVariant RsFriendListModel::displayRole(const EntryIndex& e, int col) const
{
#ifdef DEBUG_MODEL_INDEX
	std::cerr << "  Display role " << e.type << ", (" << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": ";
#endif

	switch(e.type)
	{
		case ENTRY_TYPE_GROUP:
		{
			const HierarchicalGroupInformation *group = getGroupInfo(e);

			if(!group)
				return QVariant();

			uint32_t nb_online = 0;

			for(uint32_t i=0;i<group->child_profile_indices.size();++i)
				for(uint32_t j=0;j<mProfiles[group->child_profile_indices[i]].child_node_indices.size();++j)
					if(mLocations[mProfiles[group->child_profile_indices[i]].child_node_indices[j]].node_info.state & RS_PEER_STATE_CONNECTED)
					{
						nb_online++;
						break;// only breaks the inner loop, on purpose.
					}

			switch(col)
			{
				case COLUMN_THREAD_NAME:
#ifdef DEBUG_MODEL_INDEX
					std::cerr <<   group->group_info.name.c_str() ;
#endif

					if(!group->child_profile_indices.empty())
						return QVariant(QString::fromUtf8(group->group_info.name.c_str())+" (" + QString::number(nb_online) + "/" + QString::number(group->child_profile_indices.size()) + ")");
					else
						return QVariant(QString::fromUtf8(group->group_info.name.c_str()));

                case COLUMN_THREAD_ID:  return QVariant(QString::fromStdString(group->group_info.id.toStdString()));

				default:
				return QVariant();
			}
		}
		break;

		case ENTRY_TYPE_PROFILE:
        {
                const HierarchicalProfileInformation *profile = getProfileInfo(e);

                if(!profile)
                        return QVariant();

#ifdef DEBUG_MODEL_INDEX
                std::cerr << profile->profile_info.name.c_str() ;
#endif
                switch(col)
                {
                case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(profile->profile_info.name.c_str()));
                case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(profile->profile_info.gpg_id.toStdString()) );

                case COLUMN_THREAD_IP:
                case COLUMN_THREAD_LAST_CONTACT:
                {
                        if(!isProfileExpanded(e))
                        {
                                const HierarchicalProfileInformation *hn = getProfileInfo(e);

                                QDateTime most_recent_time = QDateTime::fromTime_t(0);
                                QString most_recent_ip("---");

                                for(uint32_t i=0;i<hn->child_node_indices.size();++i)
                                {
                                        const HierarchicalNodeInformation& node = mLocations[hn->child_node_indices[i]];
                                        auto node_time = QDateTime::fromTime_t(node.node_info.lastConnect);

                                        if(most_recent_time < node_time)
                                        {
                                                most_recent_time = node_time;
                                                most_recent_ip = (node.node_info.state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(node.node_info) : QString("---");
                                        }
                                }

                                if(col == COLUMN_THREAD_LAST_CONTACT) return QVariant(most_recent_time);
                                if(col == COLUMN_THREAD_IP)           return QVariant(most_recent_ip);
                        }

                }// Fall-through
                default:
                        return QVariant();
                }
        }
		break;

		case ENTRY_TYPE_NODE:
		{
			const HierarchicalNodeInformation *node = getNodeInfo(e);

			if(!node)
				return QVariant();

#ifdef DEBUG_MODEL_INDEX
			std::cerr << node->node_info.location.c_str() ;
#endif
			switch(col)
			{
				case COLUMN_THREAD_NAME:           if(node->node_info.location.empty())
						return QVariant(QString::fromStdString(node->node_info.id.toStdString()));

				{
					std::string css = rsMsgs->getCustomStateString(node->node_info.id);

					if (mDisplayStatusString)
						if(!css.empty())
							return QVariant(QString::fromUtf8(node->node_info.location.c_str())+"\n"
						                + QString::fromUtf8(css.c_str()));
						else
						{
							return QVariant(QString::fromUtf8(node->node_info.location.c_str())+"\n"
						                + "(" + StatusDefs::name(onlineRole(e,col).toInt()) + ")");
						}
					else
						return QVariant(QString::fromUtf8(node->node_info.location.c_str()));
				}

				case COLUMN_THREAD_LAST_CONTACT:   return QVariant(QDateTime::fromTime_t(node->node_info.lastConnect).toString());
				case COLUMN_THREAD_IP:             return QVariant(  (node->node_info.state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(node->node_info) : QString("---"));
				case COLUMN_THREAD_ID:             return QVariant(  QString::fromStdString(node->node_info.id.toStdString()) );

				default:
				return QVariant();
			} break;
		}
		break;

		default: //ENTRY_TYPE
		return QVariant();
	}
}

// This function makes sure that the internal data gets updated. They are situations where the otification system cannot
// send the information about changes, such as when the computer is put on sleep.

void RsFriendListModel::checkInternalData(bool force)
{
	rstime_t now = time(NULL);

    if( (mLastInternalDataUpdate + MAX_INTERNAL_DATA_UPDATE_DELAY < now) || force)
		updateInternalData();
//    else
//    {
//        preMods();
//
//        if(mLastNodeUpdate + MAX_NODE_UPDATE_DELAY < now)
//        {
//            for(uint32_t i=0;i<mLocations.size();++i)
//                if(mLocations[i].last_update_ts + NODE_DETAILS_UPDATE_DELAY < now)
//                {
//#ifdef DEBUG_MODEL
//                    std::cerr << "Updating ID " << mLocations[i].node_info.id << std::endl;
//#endif
//                    RsPeerId id(mLocations[i].node_info.id);				// this avoids zeroing the id field when writing the node data
//                    rsPeers->getPeerDetails(id,mLocations[i].node_info);
//                    mLocations[i].last_update_ts = now;
//                }
//
//            mLastNodeUpdate = now;
//        }
//        postMods();
//    }
}

const RsFriendListModel::HierarchicalGroupInformation *RsFriendListModel::getGroupInfo(const EntryIndex& e) const
{
	if(e.group_index >= mGroups.size())
        return NULL ;
    else
        return &mGroups[e.group_index];
}

const RsFriendListModel::HierarchicalProfileInformation *RsFriendListModel::getProfileInfo(const EntryIndex& e) const
{
    // First look into the relevant group, then for the correct profile in this group.

    if(e.type != ENTRY_TYPE_PROFILE)
        return NULL ;

    if(e.group_index < UNDEFINED_GROUP_INDEX_VALUE)
    {
        const HierarchicalGroupInformation& group(mGroups[e.group_index]);

        if(e.profile_index >= group.child_profile_indices.size())
            return NULL ;

        return &mProfiles[group.child_profile_indices[e.profile_index]];
    }
    else
        return &mProfiles[e.profile_index];
}

const RsFriendListModel::HierarchicalNodeInformation *RsFriendListModel::getNodeInfo(const EntryIndex& e) const
{
	if(e.type != ENTRY_TYPE_NODE)
		return NULL ;

    uint32_t pindex = 0;

    if(e.group_index < UNDEFINED_GROUP_INDEX_VALUE)
    {
        const HierarchicalGroupInformation& group(mGroups[e.group_index]);

        if(e.profile_index >= group.child_profile_indices.size())
            return NULL ;

        pindex = group.child_profile_indices[e.profile_index];
    }
    else
    {
        if(e.profile_index >= mProfiles.size())
            return NULL ;

        pindex = e.profile_index;
	}

    if(e.node_index >= mProfiles[pindex].child_node_indices.size())
        return NULL ;

    HierarchicalNodeInformation& node(mLocations[mProfiles[pindex].child_node_indices[e.node_index]]);

	return &node;
}

bool RsFriendListModel::getPeerOnlineStatus(const EntryIndex& e) const
{
    const HierarchicalNodeInformation *noded = getNodeInfo(e) ;
    return (noded && (noded->node_info.state & RS_PEER_STATE_CONNECTED));
}

QVariant RsFriendListModel::decorationRole(const EntryIndex& entry,int col) const
{
    if(col > 0)
        return QVariant();

    switch(entry.type)
    {
    case ENTRY_TYPE_GROUP: 
	{
		const HierarchicalGroupInformation *groupInfo = getGroupInfo(entry);

		if (groupInfo->group_info.id.toStdString() == RS_GROUP_ID_FRIENDS.toStdString()) {
			return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_FRIENDS));
		}
		if (groupInfo->group_info.id.toStdString() == RS_GROUP_ID_FAMILY.toStdString()) {
			return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_FAMILY));
		}
		if (groupInfo->group_info.id.toStdString() == RS_GROUP_ID_COWORKERS.toStdString()) {
			return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_COWORKERS));
		}
		if (groupInfo->group_info.id.toStdString() == RS_GROUP_ID_OTHERS.toStdString()) {
			return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_OTHERCONTACTS));
		}
		if (groupInfo->group_info.id.toStdString() == RS_GROUP_ID_FAVORITES.toStdString()) {
			return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_FAVORITES));
		}

		return QVariant(FilesDefs::getIconFromQtResourcePath(IMAGE_OTHERGROUPS));
	}
    case ENTRY_TYPE_PROFILE:
    {
        if(!isProfileExpanded(entry))
		{
			QPixmap sslAvatar = FilesDefs::getPixmapFromQtResourcePath(AVATAR_DEFAULT_IMAGE);

        	const HierarchicalProfileInformation *hn = getProfileInfo(entry);

			for(uint32_t i=0;i<hn->child_node_indices.size();++i)
				if(AvatarDefs::getAvatarFromSslId(RsPeerId(mLocations[hn->child_node_indices[i]].node_info.id.toStdString()), sslAvatar))
					return QVariant(QIcon(sslAvatar));

            return QVariant(QIcon(sslAvatar));
		}

        return QVariant();
    }

    case ENTRY_TYPE_NODE:
    {
        const HierarchicalNodeInformation *hn = getNodeInfo(entry);

        if(!hn)
            return QVariant();

		QPixmap sslAvatar;
		AvatarDefs::getAvatarFromSslId(RsPeerId(hn->node_info.id.toStdString()), sslAvatar);

        return QVariant(QIcon(sslAvatar));
    }
    default: return QVariant();
    }
}

void RsFriendListModel::clear()
{
    preMods();

    mGroups.clear();
    mProfiles.clear();
    mLocations.clear();
    mTopLevel.clear();

	postMods();

	emit friendListChanged();
}

void RsFriendListModel::debug_dump() const
{
    std::cerr << "==== FriendListModel Debug dump ====" << std::endl;

	for(uint32_t j=0;j<mTopLevel.size();++j)
    {
        if(mTopLevel[j].type == ENTRY_TYPE_GROUP)
		{
			const HierarchicalGroupInformation& hg(mGroups[mTopLevel[j].group_index]);

			std::cerr << "Group: " << hg.group_info.name << ", ";
			std::cerr << "  children indices: " ; for(uint32_t i=0;i<hg.child_profile_indices.size();++i) std::cerr << hg.child_profile_indices[i] << " " ; std::cerr << std::endl;

			for(uint32_t i=0;i<hg.child_profile_indices.size();++i)
			{
				uint32_t profile_index = hg.child_profile_indices[i];

				std::cerr << "    Profile " << mProfiles[profile_index].profile_info.gpg_id << std::endl;

				const HierarchicalProfileInformation& hprof(mProfiles[profile_index]);

				for(uint32_t k=0;k<hprof.child_node_indices.size();++k)
					std::cerr << "      Node " << mLocations[hprof.child_node_indices[k]].node_info.id << std::endl;
			}
		}
        else if(mTopLevel[j].type == ENTRY_TYPE_PROFILE)
        {
			const HierarchicalProfileInformation& hprof(mProfiles[mTopLevel[j].profile_index]);

			std::cerr << "Profile " << hprof.profile_info.gpg_id << std::endl;

			for(uint32_t k=0;k<hprof.child_node_indices.size();++k)
				std::cerr << "  Node " << mLocations[hprof.child_node_indices[k]].node_info.id << std::endl;
        }
    }
    std::cerr << "====================================" << std::endl;
}

bool RsFriendListModel::getGroupData  (const QModelIndex& i,RsGroupInfo     & data) const
{
    if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(i.internalId(),e) || e.type != ENTRY_TYPE_GROUP)
        return false;

    const HierarchicalGroupInformation *ginfo = getGroupInfo(e);

    if(ginfo)
	{
		data = ginfo->group_info;
		return true;
	}
    else
        return false;
}
bool RsFriendListModel::getProfileData(const QModelIndex& i,RsProfileDetails& data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(i.internalId(),e) || e.type != ENTRY_TYPE_PROFILE)
        return false;

    const HierarchicalProfileInformation *gprof = getProfileInfo(e);

    if(gprof)
	{
		data = gprof->profile_info;
		return true;
	}
    else
        return false;
}
bool RsFriendListModel::getNodeData   (const QModelIndex& i,RsNodeDetails   & data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(i.internalId(),e) || e.type != ENTRY_TYPE_NODE)
        return false;

    const HierarchicalNodeInformation *gnode = getNodeInfo(e);

    if(gnode)
	{
		data = gnode->node_info;
		return true;
	}
    else
        return false;
}

RsFriendListModel::EntryType RsFriendListModel::getType(const QModelIndex& i) const
{
	if(!i.isValid())
		return ENTRY_TYPE_UNKNOWN;

	EntryIndex e;
	if(!convertInternalIdToIndex<sizeof(uintptr_t)>(i.internalId(),e))
        return ENTRY_TYPE_UNKNOWN;

    return e.type;
}

std::map<RsPgpId,uint32_t>::const_iterator RsFriendListModel::createInvalidatedProfile(const RsPgpId& _pgp_id,const RsPgpFingerprint& fpr,std::map<RsPgpId,uint32_t>& pgp_indices,std::vector<HierarchicalProfileInformation>& mProfiles)
{
    // This is necessary by the time the full fingerprint is used in PeerNetItem.

    RsPgpId pgp_id;

    if(!fpr.isNull())
		pgp_id = rsPeers->pgpIdFromFingerprint(fpr);
    else
        pgp_id = _pgp_id;

    auto it2 = pgp_indices.find(pgp_id);

    if(it2 != pgp_indices.end())
    {
        std::cerr << "(EE) asked to create an invalidated profile that already exists!" << std::endl;
        return it2;
    }

	HierarchicalProfileInformation hprof ;

    if(rsPeers->getGPGDetails(pgp_id,hprof.profile_info))
    {
        std::cerr << "(EE) asked to create an invalidated profile that already exists: " << pgp_id << std::endl;

        pgp_indices[pgp_id] = mProfiles.size();
        mProfiles.push_back(hprof);
        it2 = pgp_indices.find(pgp_id);

        return it2;
    }

    hprof.profile_info.isOnlyGPGdetail = true;
	hprof.profile_info.gpg_id = pgp_id;

	hprof.profile_info.name = tr("Profile ID ").toStdString() + pgp_id.toStdString() + tr(" (Not yet validated)").toStdString();
	hprof.profile_info.issuer = pgp_id;

	hprof.profile_info.fpr = fpr; /* pgp fingerprint */

	hprof.profile_info.trustLvl = 0;
	hprof.profile_info.validLvl = 0;

	pgp_indices[pgp_id] = mProfiles.size();
	mProfiles.push_back(hprof);

	it2 = pgp_indices.find(pgp_id);

#ifdef DEBUG_MODEL
		RsDbg() << "  Creating invalidated profile pgp id = " << pgp_id <<  " (" << hprof.profile_info.name << ") and fingerprint " << fpr << std::endl;
#endif
	return it2;
}

std::map<RsPgpId,uint32_t>::const_iterator RsFriendListModel::checkProfileIndex(const RsPgpId& pgp_id,std::map<RsPgpId,uint32_t>& pgp_indices,std::vector<HierarchicalProfileInformation>& mProfiles,bool create)
{
	auto it2 = pgp_indices.find(pgp_id);

	if(it2 == pgp_indices.end())
	{
        if(!create)
        {
            std::cerr << "(EE) trying to display profile " << pgp_id <<" that is actually not a friend." << std::endl;
            return it2;
        }
		HierarchicalProfileInformation hprof ;
		rsPeers->getGPGDetails(pgp_id,hprof.profile_info);

		pgp_indices[pgp_id] = mProfiles.size();
		mProfiles.push_back(hprof);

		it2 = pgp_indices.find(pgp_id);

#ifdef DEBUG_MODEL
		RsDbg() << "  Creating profile pgp id = " << pgp_id <<  " (" << hprof.profile_info.name << ")" << std::endl;
#endif
	}
	return it2;
}

void RsFriendListModel::updateInternalData()
{
    preMods();

    beginResetModel();

    mGroups.clear();
    mProfiles.clear();
    mLocations.clear();
    mTopLevel.clear();

    auto TL = mTopLevel ; // This allows to fill TL without touching mTopLevel outside of [begin/end]InsertRows().

    // create a map of profiles and groups
    std::map<RsPgpId,uint32_t> pgp_indices;

    // parse PGP friends that may or may not have a known location. Create the associated data.

    std::list<RsPgpId> pgp_friends;
    rsPeers->getGPGAcceptedList(pgp_friends);

    for(auto it(pgp_friends.begin());it!=pgp_friends.end();++it)
    	checkProfileIndex(*it,pgp_indices,mProfiles,true);

     // Now parse peer ids and look for the associated PGP id. If not found, raise an error.
#ifdef DEBUG_MODEL
    RsDbg() << "Updating Nodes information: " << std::endl;
#endif
    std::list<RsPeerId> peer_ids ;
    rsPeers->getFriendList(peer_ids);

    for(auto it(peer_ids.begin());it!=peer_ids.end();++it)
    {
		// profiles

        HierarchicalNodeInformation hnode ;
        rsPeers->getPeerDetails(*it,hnode.node_info);

        // If the Peer ID belong to our own profile, we add our own profile to the list. Otherwise we do not display it in the friend list.

        auto it2 = checkProfileIndex(hnode.node_info.gpg_id,pgp_indices,mProfiles,hnode.node_info.gpg_id == rsPeers->getGPGOwnId());

        if(it2 == pgp_indices.end())
        {
            // This peer's pgp key hasn't been validated yet. We list such peers at the end.

            it2 = createInvalidatedProfile(hnode.node_info.gpg_id,hnode.node_info.fpr,pgp_indices,mProfiles);
        }

		mProfiles[it2->second].child_node_indices.push_back(mLocations.size());
        mLocations.push_back(hnode);
    }


    // finally, parse groups

    if(mDisplayGroups)
	{
		// groups

		std::list<RsGroupInfo> groupInfoList;
		rsPeers->getGroupInfoList(groupInfoList) ;

#ifdef DEBUG_MODEL
		RsDbg() << "Updating Groups information: " << std::endl;
#endif

		for(auto it(groupInfoList.begin());it!=groupInfoList.end();++it)
		{
			// first, fill the group hierarchical info

			HierarchicalGroupInformation hgroup;
			hgroup.group_info = *it;

#ifdef DEBUG_MODEL
			RsDbg() << "  Group \"" << hgroup.group_info.name << "\"" << std::endl;
#endif

			for(auto it2((*it).peerIds.begin());it2!=(*it).peerIds.end();++it2)
			{
				// Then for each peer in this group, make sure that the peer is already known, and if not create it

				auto it3 = checkProfileIndex(*it2,pgp_indices,mProfiles,false);

				if(it3 == pgp_indices.end())// not found
                    continue;

				hgroup.child_profile_indices.push_back(it3->second);
			}

			mGroups.push_back(hgroup);
		}
	}

    // now  the top level list

#ifdef DEBUG_MODEL
    RsDbg() << "Creating top level list" << std::endl;
#endif

    std::set<RsPgpId> already_in_a_group;

    if(mDisplayGroups)	// in this case, we list all groups at the top level followed by the profiles without parent group
    {
        for(uint32_t i=0;i<mGroups.size();++i)
        {
#ifdef DEBUG_MODEL
			RsDbg() << "  Group " << mGroups[i].group_info.name << std::endl;
#endif

            EntryIndex e;
            e.type = ENTRY_TYPE_GROUP;
            e.group_index = i;

            TL.push_back(e);

            for(uint32_t j=0;j<mGroups[i].child_profile_indices.size();++j)
                already_in_a_group.insert(mProfiles[mGroups[i].child_profile_indices[j]].profile_info.gpg_id);
        }
    }

	for(uint32_t i=0;i<mProfiles.size();++i)
        if(already_in_a_group.find(mProfiles[i].profile_info.gpg_id)==already_in_a_group.end())
		{
#ifdef DEBUG_MODEL
			RsDbg() << "  Profile " << mProfiles[i].profile_info.name << std::endl;
#endif

			EntryIndex e;
			e.type = ENTRY_TYPE_PROFILE;
			e.profile_index = i;
            e.group_index = UNDEFINED_GROUP_INDEX_VALUE;

            TL.push_back(e);
		}

	// finally, tell the model client that layout has changed.

	mTopLevel = TL;

	if (TL.size()>0)
	{
		beginInsertRows(QModelIndex(),0,TL.size()-1);
		endInsertRows();
	}

    endResetModel();
    postMods();

	mLastInternalDataUpdate = time(NULL);
}

QModelIndex RsFriendListModel::getIndexOfGroup(const RsNodeGroupId& mid) const
{
    if(mDisplayGroups)
		for(uint32_t i=0;i<mGroups.size();++i)
			if(mGroups[i].group_info.id == mid)
                return index(i,0,QModelIndex());

    return QModelIndex();
}

void RsFriendListModel::collapseItem(const QModelIndex& index)
{
    if(getType(index) != ENTRY_TYPE_PROFILE)
        return;

    EntryIndex entry;

    if(!convertInternalIdToIndex<sizeof(uintptr_t)>(index.internalId(),entry))
        return;

    const HierarchicalProfileInformation *hp = getProfileInfo(entry);
    const HierarchicalGroupInformation *hg = getGroupInfo(entry);

    std::string s ;

    if(hg) s += hg->group_info.id.toStdString() ;
    if(hp) s += hp->profile_info.gpg_id.toStdString();

    if(!s.empty())
		mExpandedProfiles.erase(s);

    // apparently we cannot be subtle here.
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mTopLevel.size()-1,columnCount()-1,(void*)NULL));
}

void RsFriendListModel::expandItem(const QModelIndex& index)
{
    if(getType(index) != ENTRY_TYPE_PROFILE)
        return;

    EntryIndex entry;

    if(!convertInternalIdToIndex<sizeof(uintptr_t)>(index.internalId(),entry))
        return;

    const HierarchicalProfileInformation *hp = getProfileInfo(entry);
    const HierarchicalGroupInformation *hg = getGroupInfo(entry);

    std::string s ;

    if(hg) s += hg->group_info.id.toStdString() ;
    if(hp) s += hp->profile_info.gpg_id.toStdString();

    if(!s.empty())
        mExpandedProfiles.insert(s);

    // apparently we cannot be subtle here.
    emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mTopLevel.size()-1,columnCount()-1,(void*)NULL));
}

bool RsFriendListModel::isProfileExpanded(const EntryIndex& e) const
{
    if(e.type != ENTRY_TYPE_PROFILE)
        return false;

    const HierarchicalProfileInformation *hp = getProfileInfo(e);
    const HierarchicalGroupInformation *hg = getGroupInfo(e);

    std::string s ;

    if(hg) s += hg->group_info.id.toStdString() ;
    if(hp) s += hp->profile_info.gpg_id.toStdString();

    return mExpandedProfiles.find(s) != mExpandedProfiles.end();
}

