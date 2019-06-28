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
        return mGroups[index.ind].peerIds.size();

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
        return false; // TODO
    if(parent_index.type == ENTRY_TYPE_GROUP)
        return !mGroups[parent_index.ind].peerIds.empty();

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
        if(mDisplayGroups)
        {
            for(int i=0;i<mGroups.size();++i)
                if(mGroups[i].peerIds.find(mProfiles[I.ind].gpg_id) != mGroups[i].peerIds.end())	// this is costly. We should store indices
                {
					quintptr ref ;
                    EntryIndex parent_index(ENTRY_TYPE_GROUP,i);
					convertIndexToInternalId(parent_index,ref);

					return createIndex(i,0,ref);
                }
        }
		else
			return QModelIndex();

//    if(i.type == ENTRY_TYPE_NODE)
//    {
//		quintptr ref ;
//		convertIndexToInternalId(parent_index,ref);
//
//		return createIndex(parent_row,0,ref);
//    }

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
		case COLUMN_THREAD_NAME:   return QVariant(QString::fromUtf8(mGroups[e.ind].name.c_str()));

		default:
			return QVariant();
		} break;

	case ENTRY_TYPE_PROFILE:
        if(e.ind >= mProfiles.size())
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(mProfiles[e.ind].name.c_str()));
		case COLUMN_THREAD_ID:             return QVariant(QString::fromStdString(mProfiles[e.ind].gpg_id.toStdString()) );

		default:
			return QVariant();
		} break;

	case ENTRY_TYPE_NODE:
        if(e.ind >= mLocations.size())
            return QVariant();

		switch(col)
		{
		case COLUMN_THREAD_NAME:           return QVariant(QString::fromUtf8(mLocations[e.ind].location.c_str()));
		case COLUMN_THREAD_LAST_CONTACT:   return QVariant(QDateTime::fromTime_t(mLocations[e.ind].lastConnect).toString());
		case COLUMN_THREAD_IP:             return QVariant(  (mLocations[e.ind].state & RS_PEER_STATE_CONNECTED) ? StatusDefs::connectStateIpString(mLocations[e.ind]) : QString("---"));
		case COLUMN_THREAD_ID:             return QVariant(  QString::fromStdString(mLocations[e.ind].id.toStdString()) );

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
		std::cerr << "Group: " << *it << std::endl;
}

bool RsFriendListModel::getGroupData  (const QModelIndex& i,RsGroupInfo     & data) const
{
    if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_GROUP || e.ind >= mGroups.size())
        return false;

    data = mGroups[e.ind];
    return true;
}
bool RsFriendListModel::getProfileData(const QModelIndex& i,RsProfileDetails& data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_PROFILE || e.ind >= mProfiles.size())
        return false;

    data = mProfiles[e.ind];
    return true;
}
bool RsFriendListModel::getNodeData   (const QModelIndex& i,RsNodeDetails   & data) const
{
	if(!i.isValid())
        return false;

    EntryIndex e;
	if(!convertInternalIdToIndex(i.internalId(),e) || e.type != ENTRY_TYPE_NODE || e.ind >= mLocations.size())
        return false;

    data = mLocations[e.ind];
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

    // groups

    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList) ;

    for(auto it(groupInfoList.begin());it!=groupInfoList.end();++it)
        mGroups.push_back(*it);

    // profiles

    std::list<RsPgpId> gpg_ids;
	rsPeers->getGPGAcceptedList(gpg_ids);

    for(auto it(gpg_ids.begin());it!=gpg_ids.end();++it)
    {
        RsProfileDetails det;

        if(!rsPeers->getGPGDetails(*it,det))
            continue;

        mProfiles.push_back(det);
    }

    // locations

	std::list<RsPeerId> peer_ids;
	rsPeers->getFriendList(peer_ids);

    for(auto it(peer_ids.begin());it!=peer_ids.end();++it)
    {
        RsNodeDetails det;

        if(!rsPeers->getPeerDetails(*it,det))
            continue;

        mLocations.push_back(det);
    }

    postMods();
}


