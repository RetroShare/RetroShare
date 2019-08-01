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
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/common/FriendListModel.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "retroshare/rsexpr.h"
#include "retroshare/rsmsgs.h"

//#define DEBUG_MESSAGE_MODEL

#define IS_MESSAGE_UNREAD(flags) (flags &  (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER))

#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsFriendListModel::FilterString("filtered");

RsFriendListModel::RsFriendListModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    mDisplayGroups = true;
    mFilterStrings.clear();
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
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mTopLevel.size()-1,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
}

int RsFriendListModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() >= COLUMN_THREAD_NB_COLUMNS)
        return 0;

	if(parent.internalId() == 0)
		return mTopLevel.size();

    EntryIndex index;
    if(!convertInternalIdToIndex(parent.internalId(),index))
        return 0;

    if(index.type == ENTRY_TYPE_GROUP)
        return mGroups[index.group_index].child_profile_indices.size();

    if(index.type == ENTRY_TYPE_PROFILE)
		if(index.group_index < 0xff)
			return mProfiles[mGroups[index.group_index].child_profile_indices[index.profile_index]].child_node_indices.size();
        else
            return mProfiles[index.profile_index].child_node_indices.size();

    if(index.type == ENTRY_TYPE_NODE)
        return 0;
}

int RsFriendListModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

// bool RsFriendListModel::getProfileData(const QModelIndex& i,Rs::Msgs::MessageInfo& fmpe) const
// {
// 	if(!i.isValid())
//         return true;
//
//     quintptr ref = i.internalId();
// 	uint32_t index = 0;
//
// 	if(!convertInternalIdToMsgIndex(ref,index) || index >= mMessages.size())
// 		return false ;
//
// 	return rsMsgs->getMessage(mMessages[index].msgId,fmpe);
// }

bool RsFriendListModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

	EntryIndex parent_index ;
    convertInternalIdToIndex(parent.internalId(),parent_index);

    if(parent_index.type == ENTRY_TYPE_NODE)
        return false;

    if(parent_index.type == ENTRY_TYPE_PROFILE)
		if(parent_index.group_index < 0xff)
			return !mProfiles[mGroups[parent_index.group_index].child_profile_indices[parent_index.profile_index]].child_node_indices.empty();
        else
            return !mProfiles[parent_index.profile_index].child_node_indices.empty();

    if(parent_index.type == ENTRY_TYPE_GROUP)
        return !mGroups[parent_index.group_index].child_profile_indices.empty();

	return false;
}

RsFriendListModel::EntryIndex RsFriendListModel::EntryIndex::parent() const
{
    EntryIndex i(*this);

    switch(type)
    {
    case ENTRY_TYPE_GROUP: i.type = ENTRY_TYPE_TOP_LEVEL;
        				   i.group_index = 0;
						   break;

    case ENTRY_TYPE_TOP_LEVEL: i.type = ENTRY_TYPE_UNKNOWN;
        				   i.top_level_index = 0;
						   break;

    case ENTRY_TYPE_PROFILE: i.type = (i.group_index==0xff)?ENTRY_TYPE_TOP_LEVEL:ENTRY_TYPE_GROUP;
        				   i.top_level_index = 0;
						   break;

    case ENTRY_TYPE_NODE:  i.type = ENTRY_TYPE_PROFILE;
        				   i.node_index = 0;
						   break;
    }

    return i;
}

RsFriendListModel::EntryIndex RsFriendListModel::EntryIndex::child(int row,const std::vector<EntryIndex>& top_level) const
{
    EntryIndex i(*this);

	switch(type)
    {
    case ENTRY_TYPE_UNKNOWN: i.type = ENTRY_TYPE_TOP_LEVEL;
        				   i.top_level_index = row;
						   break;

    case ENTRY_TYPE_TOP_LEVEL:
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
    	case ENTRY_TYPE_UNKNOWN: return 0;
    	case ENTRY_TYPE_TOP_LEVEL: return top_level_index;
    	case ENTRY_TYPE_GROUP    : return group_index;
		case ENTRY_TYPE_PROFILE  : return (group_index==0xff)?(profile_index+nb_groups):profile_index;
    	case ENTRY_TYPE_NODE     : return node_index;
    }
}

// The index encodes the whole hierarchy of parents. This allows to very efficiently compute indices of the parent of an index.
//
// The format is the following:
//
//     0x 00 00 00 00
//        |  |  |  |
//        |  |  |  +---- location/node index
//        |  |  +------- profile index
//        |  +---------- group index
//        +------------- top level index
//
// Only valid indexes a 0x01->0xff. 0x00 means "no index"
//

bool RsFriendListModel::convertIndexToInternalId(const EntryIndex& e,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    if(e.top_level_index > 254 || e.group_index > 255 || e.profile_index > 255 || e.node_index > 255)
    {
        RsErr() << "Index out of scope in " << __PRETTY_FUNCTION__ << std::endl;
        return false;
    }

    id = ( (e.top_level_index + 1) << 24) + (e.group_index << 16) + (e.profile_index << 8) + e.node_index;
	return true;
}

bool RsFriendListModel::convertInternalIdToIndex(quintptr ref,EntryIndex& e)
{
    if(ref == 0)
        return false ;

    e.profile_index = e.node_index = e.group_index = 0xff;

    e.top_level_index = uint8_t((ref >> 24) & 0xff)-1;

    if(e.top_level_index == 0xff)
    {
        e.type = ENTRY_TYPE_UNKNOWN  ;
        return true;
    }

    e.group_index     = (ref >> 16) & 0xff;
    e.profile_index   = (ref >>  8) & 0xff;
    e.node_index      = (ref >>  0) & 0xff;

    if(e.node_index < 0xff)
        e.type = ENTRY_TYPE_NODE;
    else if(e.profile_index < 0xff)
		e.type = ENTRY_TYPE_PROFILE;
    else if(e.group_index < 0xff)
		e.type = ENTRY_TYPE_GROUP;
    else
		e.type = ENTRY_TYPE_TOP_LEVEL;

	return true;
}

QModelIndex RsFriendListModel::index(int row, int column, const QModelIndex& parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_THREAD_NB_COLUMNS)
		return QModelIndex();

    if(parent.internalId() == 0)
    {
		quintptr ref ;

		convertIndexToInternalId(EntryIndex::topLevelIndex(row),ref);
		return createIndex(row,column,ref) ;
    }

    EntryIndex parent_index ;
    convertInternalIdToIndex(parent.internalId(),parent_index);

    RsDbg() << "Index row=" << row << " col=" << column << " parent=" << parent << std::endl;

    quintptr ref;
    EntryIndex new_index = parent_index.child(row,mTopLevel);
    convertIndexToInternalId(new_index,ref);

    RsDbg() << "  returning " << createIndex(row,column,ref) << std::endl;

    return createIndex(row,column,ref);
}

QModelIndex RsFriendListModel::parent(const QModelIndex& index) const
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

	return createIndex(I.parentRow(mGroups.size()),0,i);
}

Qt::ItemFlags RsFriendListModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant RsFriendListModel::headerData(int section, Qt::Orientation orientation, int role) const
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

	switch(role)
	{
	case Qt::SizeHintRole: return sizeHintRole(index.column()) ;
    case Qt::StatusTipRole:return QVariant();
    default: break;
	}

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
	case Qt::DisplayRole:    return displayRole(entry,index.column()) ;
	default:
		return QVariant();
	}

//    if(role == Qt::FontRole)
//    {
//        QFont font ;
//		font.setBold(fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER));
//
//        return QVariant(font);
//    }

// 	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
// 	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
// 	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
// 	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
// 	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;
//
// 	case FilterRole:         return filterRole    (fmpe,index.column()) ;
// 	case StatusRole:         return statusRole    (fmpe,index.column()) ;
// 	case SortRole:           return sortRole      (fmpe,index.column()) ;
// 	case MsgFlagsRole:       return fmpe.msgflags ;
// 	case UnreadRole: 		 return fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);
// 	case MsgIdRole:          return QString::fromStdString(fmpe.msgId) ;
// 	case SrcIdRole:          return QString::fromStdString(fmpe.srcId.toStdString()) ;
}

QVariant RsFriendListModel::textColorRole(const EntryIndex& fmpe,int column) const
{
	return QVariant();
}

QVariant RsFriendListModel::statusRole(const EntryIndex& fmpe,int column) const
{
// 	if(column != COLUMN_THREAD_DATA)
//        return QVariant();

    return QVariant();//fmpe.mMsgStatus);
}

bool RsFriendListModel::passesFilter(const EntryIndex& e,int column) const
{
//     QString s ;
//     bool passes_strings = true ;
//
//     if(!mFilterStrings.empty())
// 	{
// 		switch(mFilterType)
// 		{
// 		case FILTER_TYPE_ID: 	s = displayRole(fmpe,COLUMN_THREAD_SUBJECT).toString();
// 			break;
//
// 		case FILTER_TYPE_NAME:  s = sortRole(fmpe,COLUMN_THREAD_AUTHOR).toString();
// 								if(s.isNull())
// 									passes_strings = false;
//             break;
// 		};
// 	}
//
//     if(!s.isNull())
// 		for(auto iter(mFilterStrings.begin()); iter != mFilterStrings.end(); ++iter)
// 			passes_strings = passes_strings && s.contains(*iter,Qt::CaseInsensitive);
//
//     bool passes_quick_view =
//             (mQuickViewFilter==QUICK_VIEW_ALL)
//             || (std::find(fmpe.msgtags.begin(),fmpe.msgtags.end(),mQuickViewFilter) != fmpe.msgtags.end())
//             || (mQuickViewFilter==QUICK_VIEW_STARRED && (fmpe.msgflags & RS_MSG_STAR))
//             || (mQuickViewFilter==QUICK_VIEW_SYSTEM && (fmpe.msgflags & RS_MSG_SYSTEM));
// #ifdef DEBUG_MESSAGE_MODEL
//     std::cerr << "Passes filter: type=" << mFilterType << " s=\"" << s.toStdString() << "MsgFlags=" << fmpe.msgflags << " msgtags=" ;
//     foreach(uint32_t i,fmpe.msgtags) std::cerr << i << " " ;
//     std::cerr          << "\" strings:" << passes_strings << " quick_view:" << passes_quick_view << std::endl;
// #endif
//
//     return passes_quick_view && passes_strings;
}

QVariant RsFriendListModel::filterRole(const EntryIndex& e,int column) const
{
    if(passesFilter(e,column))
        return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsFriendListModel::updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings)
{
    QString s ;
	uint32_t count = 0;

	return count;
}


void RsFriendListModel::setFilter(FilterType filter_type, const QStringList& strings)
{
    std::cerr << "Setting filter to filter_type=" << int(filter_type) << " and strings to " ;
    foreach(const QString& str,strings)
        std::cerr << "\"" << str.toStdString() << "\" " ;
    std::cerr << std::endl;

    preMods();

    mFilterType = filter_type;
	mFilterStrings = strings;

	postMods();
}

QVariant RsFriendListModel::toolTipRole(const EntryIndex& fmpe,int column) const
{
//    if(column == COLUMN_THREAD_AUTHOR)
//	{
//		QString str,comment ;
//		QList<QIcon> icons;
//
//		if(!GxsIdDetails::MakeIdDesc(RsGxsId(fmpe.srcId.toStdString()), true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
//			return QVariant();
//
//		int S = QFontMetricsF(QApplication::font()).height();
//		QImage pix( (*icons.begin()).pixmap(QSize(4*S,4*S)).toImage());
//
//		QString embeddedImage;
//		if(RsHtml::makeEmbeddedImage(pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, 8*S * 8*S))
//			comment = "<table><tr><td>" + embeddedImage + "</td><td>" + comment + "</td></table>";
//
//		return comment;
//	}

    return QVariant();
}

QVariant RsFriendListModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	switch(col)
	{
	default:
	case COLUMN_THREAD_NAME:               return QVariant( QSize(factor * 170, factor*14 ));
	case COLUMN_THREAD_IP:                 return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_ID:                 return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_LAST_CONTACT:       return QVariant( QSize(factor * 75 , factor*14 ));
	}
}

QVariant RsFriendListModel::sortRole(const EntryIndex& fmpe,int column) const
{
//    switch(column)
//    {
//	case COLUMN_THREAD_DATE:  return QVariant(QString::number(fmpe.ts)); // we should probably have leading zeroes here
//
//	case COLUMN_THREAD_READ:  return QVariant((bool)IS_MESSAGE_UNREAD(fmpe.msgflags));
//
//	case COLUMN_THREAD_STAR:  return QVariant((fmpe.msgflags & RS_MSG_STAR)? 1:0);
//
//    case COLUMN_THREAD_AUTHOR:{
//        						QString name;
//
//        						if(GxsIdTreeItemDelegate::computeName(RsGxsId(fmpe.srcId.toStdString()),name))
//                                    return name;
//    }
//    default:
//        return displayRole(fmpe,column);
//    }
    return QVariant();
}

QVariant RsFriendListModel::displayRole(const EntryIndex& e, int col) const
{
    std::cerr << "  Display role " << e.type << ", (" << (int)e.top_level_index << "," << (int)e.group_index << ","<< (int)e.profile_index << ","<< (int)e.node_index << ") col="<< col<<": " << std::endl;

    switch(e.type)
	{
	case ENTRY_TYPE_GROUP:
        {
  	      const RsGroupInfo *group = getGroupInfo(e);

  	      if(!group)
  	          return QVariant();

			switch(col)
			{
			case COLUMN_THREAD_NAME:
              	std::cerr <<   group->name.c_str() << std::endl;
                return QVariant(QString::fromUtf8(group->name.c_str()));

			default:
				return QVariant();
			}
    	}
        break;

	case ENTRY_TYPE_PROFILE:
		{
 	       const RsProfileDetails *profile = getProfileInfo(e);

 	       if(!profile)
 	           return QVariant();

			switch(col)
			{
			case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(profile->name.c_str()));
			case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(profile->gpg_id.toStdString()) );

			default:
				return QVariant();
			}
       }
        break;

	case ENTRY_TYPE_NODE:
        const RsNodeDetails *node = getNodeInfo(e);

        if(!node)
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(node->location.c_str()));
		case COLUMN_THREAD_LAST_CONTACT:   return QVariant(QDateTime::fromTime_t(node->lastConnect).toString());
		case COLUMN_THREAD_IP:             return QVariant(  (node->state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(*node) : QString("---"));
		case COLUMN_THREAD_ID:             return QVariant(  QString::fromStdString(node->id.toStdString()) );

		default:
			return QVariant();
		} break;

		return QVariant();
	}
}

const RsGroupInfo *RsFriendListModel::getGroupInfo(const EntryIndex& e) const
{
	if(e.group_index >= mGroups.size())
        return NULL ;
    else
        return &mGroups[e.group_index].group_info;
}

const RsFriendListModel::RsProfileDetails *RsFriendListModel::getProfileInfo(const EntryIndex& e) const
{
    // First look into the relevant group, then for the correct profile in this group.

    if(e.type != ENTRY_TYPE_PROFILE)
        return NULL ;

    if(e.top_level_index >= mTopLevel.size())
        return NULL ;

    if(e.group_index < 0xff)
    {
        const HierarchicalGroupInformation& group(mGroups[e.group_index]);

        if(e.group_index >= group.child_profile_indices.size())
            return NULL ;

        return &mProfiles[group.child_profile_indices[e.group_index]].profile_info;
    }
    else
    {
        if(e.group_index >= mProfiles.size())
            return NULL ;

        return &mProfiles[e.group_index].profile_info;
	}
}

const RsFriendListModel::RsNodeDetails *RsFriendListModel::getNodeInfo(const EntryIndex& e) const
{
	if(e.type != ENTRY_TYPE_NODE)
		return NULL ;

    if(e.top_level_index >= mTopLevel.size())
        return NULL ;

    uint32_t pindex = 0;

    if(e.group_index < 0xff)
    {
        const HierarchicalGroupInformation& group(mGroups[e.group_index]);

        if(e.group_index >= group.child_profile_indices.size())
            return NULL ;

        pindex = group.child_profile_indices[e.group_index];
    }
    else
    {
        if(e.group_index >= mProfiles.size())
            return NULL ;

        pindex = e.group_index;
	}

    if(e.node_index >= mProfiles[pindex].child_node_indices.size())
        return NULL ;

    return &mLocations[mProfiles[pindex].child_node_indices[e.node_index]].node_info;
}


QVariant RsFriendListModel::userRole(const EntryIndex& fmpe,int col) const
{
//	switch(col)
//    {
//     	case COLUMN_THREAD_AUTHOR:   return QVariant(QString::fromStdString(fmpe.srcId.toStdString()));
//     	case COLUMN_THREAD_MSGID:    return QVariant(QString::fromStdString(fmpe.msgId));
//    default:
//        return QVariant();
//    }

    return QVariant();
}

QVariant RsFriendListModel::decorationRole(const EntryIndex& fmpe,int col) const
{
//	if(col == COLUMN_THREAD_READ)
//		if(fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER))
//			return QIcon(":/images/message-state-unread.png");
//		else
//			return QIcon(":/images/message-state-read.png");
//
//    if(col == COLUMN_THREAD_SUBJECT)
//    {
//        if(fmpe.msgflags & RS_MSG_NEW         )  return QIcon(":/images/message-state-new.png");
//        if(fmpe.msgflags & RS_MSG_USER_REQUEST)  return QIcon(":/images/user/user_request16.png");
//        if(fmpe.msgflags & RS_MSG_FRIEND_RECOMMENDATION) return QIcon(":/images/user/friend_suggestion16.png");
//        if(fmpe.msgflags & RS_MSG_PUBLISH_KEY) return QIcon(":/images/share-icon-16.png");
//
//        if(fmpe.msgflags & RS_MSG_UNREAD_BY_USER)
//        {
//            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED)    return QIcon(":/images/message-mail-replied.png");
//            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED)  return QIcon(":/images/message-mail-forwarded.png");
//            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) return QIcon(":/images/message-mail-replied-forw.png");
//
//            return QIcon(":/images/message-mail.png");
//        }
//		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED)    return QIcon(":/images/message-mail-replied-read.png");
//		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED)  return QIcon(":/images/message-mail-forwarded-read.png");
//		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) return QIcon(":/images/message-mail-replied-forw-read.png");
//
//		return QIcon(":/images/message-mail-read.png");
//    }
//
//    if(col == COLUMN_THREAD_STAR)
//        return QIcon((fmpe.msgflags & RS_MSG_STAR) ? (IMAGE_STAR_ON ): (IMAGE_STAR_OFF));
//
//    bool isNew = fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);
//
//    if(col == COLUMN_THREAD_READ)
//        return QIcon(isNew ? ":/images/message-state-unread.png": ":/images/message-state-read.png");

	return QVariant();
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

// void RsFriendListModel::setMessages(const std::list<Rs::Msgs::MsgInfoSummary>& msgs)
// {
//     preMods();
//
//     beginRemoveRows(QModelIndex(),0,mMessages.size()-1);
//     endRemoveRows();
//
//     mMessages.clear();
//     mMessagesMap.clear();
//
//     for(auto it(msgs.begin());it!=msgs.end();++it)
//     {
//         mMessagesMap[(*it).msgId] = mMessages.size();
//     	mMessages.push_back(*it);
//     }
//
//     // now update prow for all posts
//
// #ifdef DEBUG_MESSAGE_MODEL
//     debug_dump();
// #endif
//
//     beginInsertRows(QModelIndex(),0,mMessages.size()-1);
//     endInsertRows();
// 	postMods();
//
// 	emit messagesLoaded();
// }
//
// void RsFriendListModel::updateMessages()
// {
//     emit messagesAboutToLoad();
//
//     std::list<Rs::Msgs::MsgInfoSummary> msgs;
//
//     getMessageSummaries(mCurrentBox,msgs);
// 	setMessages(msgs);
//
//     emit messagesLoaded();
// }

static bool decreasing_time_comp(const std::pair<time_t,RsGxsMessageId>& e1,const std::pair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void RsFriendListModel::debug_dump() const
{
	for(uint32_t j=0;j<mGroups.size();++j)
	{
		std::cerr << "Group: " << mGroups[j].group_info.name << ", ";
		std::cerr << "  children indices: " ; for(uint32_t i=0;i<mGroups[j].child_profile_indices.size();++i) std::cerr << mGroups[j].child_profile_indices[i] << " " ; std::cerr << std::endl;

		for(uint32_t i=0;i<mGroups[j].child_profile_indices.size();++i)
		{
			uint32_t profile_index = mGroups[j].child_profile_indices[i];

			std::cerr << "    Profile " << mProfiles[profile_index].profile_info.gpg_id << std::endl;

			const HierarchicalProfileInformation& hprof(mProfiles[profile_index]);

			for(uint32_t k=0;k<hprof.child_node_indices.size();++k)
				std::cerr << "      Node " << mLocations[hprof.child_node_indices[k]].node_info.id << std::endl;
		}

	}
}

bool RsFriendListModel::getGroupData  (const QModelIndex& i,RsGroupInfo     & data) const
{
    if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_GROUP)
        return false;

    const RsGroupInfo *ginfo = getGroupInfo(e);

    if(ginfo)
	{
		data = *ginfo;
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
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_PROFILE)
        return false;

    const RsProfileDetails *gprof = getProfileInfo(e);

    if(gprof)
	{
		data = *gprof;
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
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_NODE)
        return false;

    const RsNodeDetails *gnode = getNodeInfo(e);

    if(gnode)
	{
		data = *gnode;
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
	if(!convertInternalIdToIndex(i.internalId(),e))
        return ENTRY_TYPE_UNKNOWN;

    return e.type;
}

void RsFriendListModel::updateInternalData()
{
    preMods();

    beginRemoveRows(QModelIndex(),0,mTopLevel.size()-1);
    endRemoveRows();

    mGroups.clear();
    mProfiles.clear();
    mLocations.clear();

    mTopLevel.clear();

    // create a map of profiles and groups
    std::map<RsPgpId,      uint32_t> pgp_indices;

    // we start from the base and fill all locations in an array

    // peer ids

    RsDbg() << "Updating Nodes information: " << std::endl;

    std::list<RsPeerId> peer_ids ;
    rsPeers->getFriendList(peer_ids);

    for(auto it(peer_ids.begin());it!=peer_ids.end();++it)
    {
		// profiles

        HierarchicalNodeInformation hnode ;
        rsPeers->getPeerDetails(*it,hnode.node_info);

        auto it2 = pgp_indices.find(hnode.node_info.gpg_id);

        if(it2 == pgp_indices.end())
        {
            HierarchicalProfileInformation hprof ;
            rsPeers->getGPGDetails(hnode.node_info.gpg_id,hprof.profile_info);

            pgp_indices[hnode.node_info.gpg_id] = mProfiles.size();
            mProfiles.push_back(hprof);

			it2 = pgp_indices.find(hnode.node_info.gpg_id);
        }
		mProfiles[it2->second].child_node_indices.push_back(mLocations.size());

		RsDbg() << "  Peer " << *it << " pgp id = " << hnode.node_info.gpg_id << std::endl;

        mLocations.push_back(hnode);
    }

    // groups

    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList) ;

    RsDbg() << "Updating Groups information: " << std::endl;

    for(auto it(groupInfoList.begin());it!=groupInfoList.end();++it)
    {
		// first, fill the group hierarchical info

        HierarchicalGroupInformation hgroup;
        hgroup.group_info = *it;

		RsDbg() << "  Group \"" << hgroup.group_info.name << "\"" << std::endl;

        for(auto it2((*it).peerIds.begin());it2!=(*it).peerIds.end();++it2)
        {
            // Then for each peer in this group, make sure that the peer is already known, and if not create it

            auto it3 = pgp_indices.find(*it2);

            if(it3 == pgp_indices.end())// not found
                RsErr() << "Inconsistency error!" << std::endl;

            hgroup.child_profile_indices.push_back(it3->second);
        }

        mGroups.push_back(hgroup);
    }

    // now  the top level list

    mTopLevel.clear();
    std::set<RsPgpId> already_in_a_group;

    if(mDisplayGroups)	// in this case, we list all groups at the top level followed by the profiles without parent group
    {
        for(uint32_t i=0;i<mGroups.size();++i)
        {
            EntryIndex e;
            e.type = ENTRY_TYPE_GROUP;
            e.group_index = i;
            e.top_level_index = mTopLevel.size();

            mTopLevel.push_back(e);
        }
    }

	for(uint32_t i=0;i<mProfiles.size();++i)
        if(already_in_a_group.find(mProfiles[i].profile_info.gpg_id)==already_in_a_group.end())
		{
			EntryIndex e;
			e.type = ENTRY_TYPE_PROFILE;
			e.top_level_index = mTopLevel.size();
			e.profile_index = i;
			mTopLevel.push_back(e);
		}

    // finally, tell the model client that layout has changed.

    beginInsertRows(QModelIndex(),0,mGroups.size()-1);
    endInsertRows();

    postMods();
}


