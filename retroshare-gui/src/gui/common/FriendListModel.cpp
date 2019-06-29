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
    mDisplayGroups = false;
    mFilterStrings.clear();
}

void RsFriendListModel::setDisplayGroups(bool b)
{
    mDisplayGroups = b;

    // should update here
}
void RsFriendListModel::preMods()
{
 	emit layoutAboutToBeChanged();
}
void RsFriendListModel::postMods()
{
    if(mDisplayGroups)
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mGroups.size()-1,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
    else
		emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mProfiles.size()-1,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
}

int RsFriendListModel::rowCount(const QModelIndex& parent) const
{
    if(parent.column() >= COLUMN_THREAD_NB_COLUMNS)
        return 0;

	if(parent.internalId() == 0)
		if(mDisplayGroups)
			return mGroups.size();
		else
			return mProfiles.size();

    EntryIndex index;
    if(!convertInternalIdToIndex(parent.internalId(),index))
        return 0;

    if(index.type == ENTRY_TYPE_GROUP)
        return mGroups[index.ind].child_indices.size();

    if(index.type == ENTRY_TYPE_PROFILE)
        return mProfiles[index.ind].child_indices.size();

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
        return !mProfiles[parent_index.ind].child_indices.empty();
    if(parent_index.type == ENTRY_TYPE_GROUP)
        return !mGroups[parent_index.ind].child_indices.empty();

	return false;
}

bool RsFriendListModel::convertIndexToInternalId(const EntryIndex& e,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    if(e.ind > 0x0fffffff)
        return false;

    if(e.type == ENTRY_TYPE_GROUP)
		id = e.ind + 0x10000000 ;
    else if(e.type == ENTRY_TYPE_PROFILE)
		id = e.ind + 0x20000000 ;
    else if(e.type == ENTRY_TYPE_NODE)
		id = e.ind + 0x30000000 ;
    else
        return false;

	return true;
}

bool RsFriendListModel::convertInternalIdToIndex(quintptr ref,EntryIndex& e)
{
    if(ref == 0)
        return false ;

    e.ind = ref & 0x0fffffff;

    int t = (ref >> 28)&0xf;

    if(t == 0x1)
        e.type = ENTRY_TYPE_GROUP ;
    else if(t==0x2)
        e.type = ENTRY_TYPE_PROFILE ;
    else if(t==0x3)
        e.type = ENTRY_TYPE_NODE ;
    else
        return false;

	return true;
}

QModelIndex RsFriendListModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_THREAD_NB_COLUMNS)
		return QModelIndex();

    if(parent.internalId() == 0)
    {
		quintptr ref ;

        if(mDisplayGroups)
			convertIndexToInternalId(EntryIndex(ENTRY_TYPE_GROUP,row),ref);
		else
			convertIndexToInternalId(EntryIndex(ENTRY_TYPE_PROFILE,row),ref);

		return createIndex(row,column,ref) ;
    }

    EntryIndex parent_index ;
    EntryIndex new_index;

    convertInternalIdToIndex(parent.internalId(),parent_index);

    new_index.ind = row;

    switch(parent_index.type)
    {
    case ENTRY_TYPE_GROUP:   new_index.type = ENTRY_TYPE_PROFILE;
    case ENTRY_TYPE_PROFILE: new_index.type = ENTRY_TYPE_NODE;
    default:
        return QModelIndex();
    }
	quintptr ref ;
    convertIndexToInternalId(new_index,ref);

    return createIndex(row,column,ref);
}

QModelIndex RsFriendListModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

	EntryIndex I ;
    convertInternalIdToIndex(index.internalId(),I);

    if(I.type == ENTRY_TYPE_GROUP)
        return QModelIndex();

    if(I.type == ENTRY_TYPE_PROFILE)
	{
        quintptr ref=0;
        convertIndexToInternalId( EntryIndex( ENTRY_TYPE_GROUP, mProfiles[I.ind].parent_group_index ), ref);

        return createIndex( mProfiles[I.ind].parent_row,0,ref);
    }

    if(I.type == ENTRY_TYPE_NODE)
    {
		quintptr ref=0 ;
		convertIndexToInternalId(EntryIndex( ENTRY_TYPE_PROFILE,mLocations[I.ind].parent_profile_index),ref);

		return createIndex( mLocations[I.ind].parent_row,0,ref);
    }

	return QModelIndex();
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
    switch(e.type)
	{
	case ENTRY_TYPE_GROUP:
        if(e.ind >= mGroups.size())
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:   return QVariant(QString::fromUtf8(mGroups[e.ind].group.name.c_str()));

		default:
			return QVariant();
		} break;

	case ENTRY_TYPE_PROFILE:
        if(e.ind >= mProfiles.size())
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(mProfileDetails[mProfiles[e.ind].profile_index].name.c_str()));
		case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(mProfileDetails[mProfiles[e.ind].profile_index].gpg_id.toStdString()) );

		default:
			return QVariant();
		} break;

	case ENTRY_TYPE_NODE:
        if(e.ind >= mLocations.size())
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(mNodeDetails[mLocations[e.ind].node_index].location.c_str()));
		case COLUMN_THREAD_LAST_CONTACT:   return QVariant(QDateTime::fromTime_t(mNodeDetails[mLocations[e.ind].node_index].lastConnect).toString());
		case COLUMN_THREAD_IP:             return QVariant(  (mNodeDetails[mLocations[e.ind].node_index].state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(mNodeDetails[mLocations[e.ind].node_index]) : QString("---"));
		case COLUMN_THREAD_ID:             return QVariant(  QString::fromStdString(mNodeDetails[mLocations[e.ind].node_index].id.toStdString()) );

		default:
			return QVariant();
		} break;

		return QVariant();
	}
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
    for(auto it(mGroups.begin());it!=mGroups.end();++it)
    {
		std::cerr << "Group: " << (*it).group.name << std::endl;
    }
}

bool RsFriendListModel::getGroupData  (const QModelIndex& i,RsGroupInfo     & data) const
{
    if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_GROUP || e.ind >= mGroups.size())
        return false;

    data = mGroups[e.ind].group;
    return true;
}
bool RsFriendListModel::getProfileData(const QModelIndex& i,RsProfileDetails& data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_PROFILE || e.ind >= mProfiles.size())
        return false;

    data = mProfileDetails[mProfiles[e.ind].profile_index];
    return true;
}
bool RsFriendListModel::getNodeData   (const QModelIndex& i,RsNodeDetails   & data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_NODE || e.ind >= mLocations.size())
        return false;

    data = mNodeDetails[mLocations[e.ind].node_index];
    return true;
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

    mGroups.clear();
    mLocations.clear();
    mProfiles.clear();

    mNodeDetails.clear();
    mProfileDetails.clear();

    // create a map of profiles and groups
    std::map<RsPgpId,uint32_t> pgp_indexes;
    std::map<RsPeerId,uint32_t> ssl_indexes;
    std::map<RsNodeGroupId,uint32_t> grp_indexes;

    // we start from the top and fill in the blanks as needed

	// groups

    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList) ;
    uint32_t group_row = 0;

    for(auto it(groupInfoList.begin());it!=groupInfoList.end();++it,++group_row)
    {
		// first, fill the group hierarchical info

        HierarchicalGroupInformation groupinfo;
        groupinfo.group = *it;
        groupinfo.parent_row = group_row;

        uint32_t profile_row = 0;

        for(auto it2((*it).peerIds.begin());it2!=(*it).peerIds.end();++it2,++profile_row)
        {
            // Then for each peer in this group, make sure that the peer is already known, and if not create it

            auto it3 = pgp_indexes.find(*it2);
            if(it3 == pgp_indexes.end())// not found
            {
                RsProfileDetails profdet;

                rsPeers->getGPGDetails(*it2,profdet);

                pgp_indexes[*it2] = mProfileDetails.size();
                mProfileDetails.push_back(profdet);

				it3 = pgp_indexes.find(*it2);
            }

            // ...and also fill the hierarchical profile info

            HierarchicalProfileInformation profinfo;

            profinfo.parent_row = profile_row;
            profinfo.parent_group_index = mGroups.size();
            profinfo.profile_index = it3->second;

			// now fill the children nodes of the profile

			std::list<RsPeerId> ssl_ids ;
			rsPeers->getAssociatedSSLIds(*it2, ssl_ids);

            uint32_t node_row = 0;

			for(auto it4(ssl_ids.begin());it4!=ssl_ids.end();++it4,++node_row)
			{
				auto it5 = ssl_indexes.find(*it4);
				if(it5 == ssl_indexes.end())
                {
					RsNodeDetails nodedet;

					rsPeers->getPeerDetails(*it4,nodedet);

					ssl_indexes[*it4] = mNodeDetails.size();
					mNodeDetails.push_back(nodedet);

					it5 = ssl_indexes.find(*it4);
                }

                HierarchicalNodeInformation nodeinfo;
                nodeinfo.parent_row = node_row;
                nodeinfo.node_index = it5->second;
                nodeinfo.parent_profile_index = mProfiles.size();

                profinfo.child_indices.push_back(mLocations.size());

				mLocations.push_back(nodeinfo);
            }

            mProfiles.push_back(profinfo);
        }

        mGroups.push_back(groupinfo);
    }

    postMods();
}


