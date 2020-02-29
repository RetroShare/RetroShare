/*******************************************************************************
 * retroshare-gui/src/gui/msgs/RsMessageModel.cpp                              *
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

#include "gui/common/TagDefs.h"
#include "util/HandleRichText.h"
#include "util/DateTime.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "MessageModel.h"
#include "retroshare/rsexpr.h"
#include "retroshare/rsmsgs.h"

//#define DEBUG_MESSAGE_MODEL

#define IS_MESSAGE_UNREAD(flags) (flags &  (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER))

#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"

std::ostream& operator<<(std::ostream& o, const QModelIndex& i);// defined elsewhere

const QString RsMessageModel::FilterString("filtered");

RsMessageModel::RsMessageModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    mCurrentBox = BOX_NONE;
    mQuickViewFilter = QUICK_VIEW_ALL;
    mFilterType = FILTER_TYPE_NONE;
    mFilterStrings.clear();
}

void RsMessageModel::preMods()
{
 	emit layoutAboutToBeChanged();
}
void RsMessageModel::postMods()
{
	emit dataChanged(createIndex(0,0,(void*)NULL), createIndex(mMessages.size()-1,COLUMN_THREAD_NB_COLUMNS-1,(void*)NULL));
}

int RsMessageModel::rowCount(const QModelIndex& parent) const
{
	if(!parent.isValid())
        return 0;

    if(parent.column() > 0)
        return 0;

    if(mMessages.empty())	// security. Should never happen.
        return 0;

	if(parent.internalPointer() == NULL)
		return mMessages.size();

    return 0;
}

int RsMessageModel::columnCount(const QModelIndex &parent) const
{
	return COLUMN_THREAD_NB_COLUMNS ;
}

bool RsMessageModel::getMessageData(const QModelIndex& i,Rs::Msgs::MessageInfo& fmpe) const
{
	if(!i.isValid())
        return true;

    quintptr ref = i.internalId();
	uint32_t index = 0;

	if(!convertInternalIdToMsgIndex(ref,index) || index >= mMessages.size())
		return false ;

	return rsMsgs->getMessage(mMessages[index].msgId,fmpe);
}

bool RsMessageModel::hasChildren(const QModelIndex &parent) const
{
    if(!parent.isValid())
        return true;

    return false ;
}

bool RsMessageModel::convertMsgIndexToInternalId(uint32_t index,quintptr& id)
{
	// the internal id is set to the place in the table of items. We simply shift to allow 0 to mean something special.

    id = index + 1 ;

	return true;
}

bool RsMessageModel::convertInternalIdToMsgIndex(quintptr ref,uint32_t& index)
{
    if(ref == 0)
        return false ;

    index = ref - 1;
	return true;
}

QModelIndex RsMessageModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= COLUMN_THREAD_NB_COLUMNS)
		return QModelIndex();

	quintptr ref ;

    if(parent.internalId() == 0 && convertMsgIndexToInternalId(row,ref))
		return createIndex(row,column,ref) ;

    return QModelIndex();
}

QModelIndex RsMessageModel::parent(const QModelIndex& index) const
{
    if(!index.isValid())
        return QModelIndex();

	return QModelIndex();
}

Qt::ItemFlags RsMessageModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return 0;

    return QAbstractItemModel::flags(index);
}

QVariant RsMessageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(role == Qt::DisplayRole)
		switch(section)
		{
		case COLUMN_THREAD_DATE:         return tr("Date");
		case COLUMN_THREAD_AUTHOR:       return tr("From");
		case COLUMN_THREAD_SUBJECT:      return tr("Subject");
		case COLUMN_THREAD_TAGS:         return tr("Tags");
		default:
			return QVariant();
		}

	if(role == Qt::DecorationRole)
		switch(section)
		{
		case COLUMN_THREAD_STAR:         return QIcon(IMAGE_STAR_ON);
		case COLUMN_THREAD_READ:         return QIcon(":/images/message-state-header.png");
		case COLUMN_THREAD_ATTACHMENT:   return QIcon(":/icons/png/attachements.png");
		default:
			return QVariant();
		}

	if(role == Qt::ToolTipRole)
        switch(section)
        {
        case COLUMN_THREAD_ATTACHMENT: return tr("Click to sort by attachments");
        case COLUMN_THREAD_SUBJECT:    return tr("Click to sort by subject");
        case COLUMN_THREAD_READ:       return tr("Click to sort by read");
        case COLUMN_THREAD_AUTHOR:     return tr("Click to sort by from");
        case COLUMN_THREAD_DATE:       return tr("Click to sort by date");
        case COLUMN_THREAD_TAGS:       return tr("Click to sort by tags");
        case COLUMN_THREAD_STAR:       return tr("Click to sort by star");
		default:
			return QVariant();
        }
	return QVariant();
}

QVariant RsMessageModel::data(const QModelIndex &index, int role) const
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
	uint32_t entry = 0;

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

	if(!convertInternalIdToMsgIndex(ref,entry) || entry >= mMessages.size())
	{
#ifdef DEBUG_MESSAGE_MODEL
		std::cerr << "Bad pointer: " << (void*)ref << std::endl;
#endif
		return QVariant() ;
	}

	const Rs::Msgs::MsgInfoSummary& fmpe(mMessages[entry]);

    if(role == Qt::FontRole)
    {
        QFont font ;
		font.setBold(fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER));

        return QVariant(font);
    }

#ifdef DEBUG_MESSAGE_MODEL
	std::cerr << " [ok]" << std::endl;
#endif

	switch(role)
	{
	case Qt::DisplayRole:    return displayRole   (fmpe,index.column()) ;
	case Qt::DecorationRole: return decorationRole(fmpe,index.column()) ;
	case Qt::ToolTipRole:	 return toolTipRole   (fmpe,index.column()) ;
	case Qt::UserRole:	 	 return userRole      (fmpe,index.column()) ;
	case Qt::TextColorRole:  return textColorRole (fmpe,index.column()) ;
	case Qt::BackgroundRole: return backgroundRole(fmpe,index.column()) ;

	case FilterRole:         return filterRole    (fmpe,index.column()) ;
	case StatusRole:         return statusRole    (fmpe,index.column()) ;
	case SortRole:           return sortRole      (fmpe,index.column()) ;
	case MsgFlagsRole:       return fmpe.msgflags ;
	case UnreadRole: 		 return fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);
	case MsgIdRole:          return QString::fromStdString(fmpe.msgId) ;
	case SrcIdRole:          return QString::fromStdString(fmpe.srcId.toStdString()) ;
	default:
		return QVariant();
	}
}

QVariant RsMessageModel::textColorRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
	Rs::Msgs::MsgTagType tags;
	rsMsgs->getMessageTagTypes(tags);

    for(auto it(fmpe.msgtags.begin());it!=fmpe.msgtags.end();++it)
        for(auto it2(tags.types.begin());it2!=tags.types.end();++it2)
            if(it2->first == *it)
                return QColor(it2->second.second);

	return QVariant();
}

QVariant RsMessageModel::statusRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
// 	if(column != COLUMN_THREAD_DATA)
//        return QVariant();

    return QVariant();//fmpe.mMsgStatus);
}

bool RsMessageModel::passesFilter(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    QString s ;
    bool passes_strings = true ;

    if(!mFilterStrings.empty())
	{
		switch(mFilterType)
		{
		case FILTER_TYPE_SUBJECT: 	s = displayRole(fmpe,COLUMN_THREAD_SUBJECT).toString();
			break;

		case FILTER_TYPE_FROM:      s = sortRole(fmpe,COLUMN_THREAD_AUTHOR).toString();
            						if(s.isNull())
                                        passes_strings = false;
            break;
		case FILTER_TYPE_DATE:   	s = displayRole(fmpe,COLUMN_THREAD_DATE).toString();
			break;
		case FILTER_TYPE_CONTENT:   {
			Rs::Msgs::MessageInfo minfo;
			rsMsgs->getMessage(fmpe.msgId,minfo);
			s = QTextDocument(QString::fromUtf8(minfo.msg.c_str())).toPlainText();
		}
			break;
		case FILTER_TYPE_TAGS:		s = displayRole(fmpe,COLUMN_THREAD_TAGS).toString();
			break;

		case FILTER_TYPE_ATTACHMENTS:
		{
			Rs::Msgs::MessageInfo minfo;
			rsMsgs->getMessage(fmpe.msgId,minfo);

			for(auto it(minfo.files.begin());it!=minfo.files.end();++it)
				s += QString::fromUtf8((*it).fname.c_str())+" ";
		}
		};
	}

    if(!s.isNull())
		for(auto iter(mFilterStrings.begin()); iter != mFilterStrings.end(); ++iter)
			passes_strings = passes_strings && s.contains(*iter,Qt::CaseInsensitive);

    bool passes_quick_view =
            (mQuickViewFilter==QUICK_VIEW_ALL)
            || (std::find(fmpe.msgtags.begin(),fmpe.msgtags.end(),mQuickViewFilter) != fmpe.msgtags.end())
            || (mQuickViewFilter==QUICK_VIEW_STARRED && (fmpe.msgflags & RS_MSG_STAR))
            || (mQuickViewFilter==QUICK_VIEW_SYSTEM && (fmpe.msgflags & RS_MSG_SYSTEM));
#ifdef DEBUG_MESSAGE_MODEL
    std::cerr << "Passes filter: type=" << mFilterType << " s=\"" << s.toStdString() << "MsgFlags=" << fmpe.msgflags << " msgtags=" ;
    foreach(uint32_t i,fmpe.msgtags) std::cerr << i << " " ;
    std::cerr          << "\" strings:" << passes_strings << " quick_view:" << passes_quick_view << std::endl;
#endif

    return passes_quick_view && passes_strings;
}

QVariant RsMessageModel::filterRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    if(passesFilter(fmpe,column))
        return QVariant(FilterString);

	return QVariant(QString());
}

uint32_t RsMessageModel::updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings)
{
    QString s ;
	uint32_t count = 0;

	return count;
}


void RsMessageModel::setFilter(FilterType filter_type, const QStringList& strings)
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

QVariant RsMessageModel::toolTipRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    if(column == COLUMN_THREAD_AUTHOR)
	{
		QString str,comment ;
		QList<QIcon> icons;

		if(!GxsIdDetails::MakeIdDesc(RsGxsId(fmpe.srcId.toStdString()), true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
			return QVariant();

		int S = QFontMetricsF(QApplication::font()).height();
		QImage pix( (*icons.begin()).pixmap(QSize(4*S,4*S)).toImage());

		QString embeddedImage;
		if(RsHtml::makeEmbeddedImage(pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, 8*S * 8*S))
			comment = "<table><tr><td>" + embeddedImage + "</td><td>" + comment + "</td></table>";

		return comment;
	}

    return QVariant();
}

QVariant RsMessageModel::backgroundRole(const Rs::Msgs::MsgInfoSummary &fmpe, int column) const
{
    return QVariant();
}

QVariant RsMessageModel::sizeHintRole(int col) const
{
	float factor = QFontMetricsF(QApplication::font()).height()/14.0f ;

	switch(col)
	{
	default:
	case COLUMN_THREAD_SUBJECT:      return QVariant( QSize(factor * 170, factor*14 ));
	case COLUMN_THREAD_DATE:         return QVariant( QSize(factor * 75 , factor*14 ));
	case COLUMN_THREAD_AUTHOR:       return QVariant( QSize(factor * 75 , factor*14 ));
	}
}

QVariant RsMessageModel::authorRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    return QVariant();
}

QVariant RsMessageModel::sortRole(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const
{
    switch(column)
    {
	case COLUMN_THREAD_DATE:  return QVariant(QString::number(fmpe.ts)); // we should probably have leading zeroes here

	case COLUMN_THREAD_READ:  return QVariant((bool)IS_MESSAGE_UNREAD(fmpe.msgflags));

	case COLUMN_THREAD_STAR:  return QVariant((fmpe.msgflags & RS_MSG_STAR)? 1:0);

    case COLUMN_THREAD_AUTHOR:{
        						QString name;

        						if(GxsIdTreeItemDelegate::computeName(RsGxsId(fmpe.srcId.toStdString()),name))
                                    return name;
    }
    default:
        return displayRole(fmpe,column);
    }
}

QVariant RsMessageModel::displayRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
	switch(col)
	{
	case COLUMN_THREAD_SUBJECT:   return QVariant(QString::fromUtf8(fmpe.title.c_str()));
	case COLUMN_THREAD_ATTACHMENT:return QVariant(QString::number(fmpe.count));

    case COLUMN_THREAD_STAR:
	case COLUMN_THREAD_READ:return QVariant();
	case COLUMN_THREAD_DATE:{
		QDateTime qtime;
		qtime.setTime_t(fmpe.ts);

		return QVariant(DateTime::formatDateTime(qtime));
	}

	case COLUMN_THREAD_TAGS:{
        // Tags
        Rs::Msgs::MsgTagInfo tagInfo;
        rsMsgs->getMessageTag(fmpe.msgId, tagInfo);

        Rs::Msgs::MsgTagType Tags;
        rsMsgs->getMessageTagTypes(Tags);

        QString text;

        // build tag names
        std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
        for (auto tagit = tagInfo.tagIds.begin(); tagit != tagInfo.tagIds.end(); ++tagit)
        {
            if (!text.isNull())
                text += ",";

            auto Tag = Tags.types.find(*tagit);

            if (Tag != Tags.types.end())
                text += TagDefs::name(Tag->first, Tag->second.first);
            else
                std::cerr << "(WW) unknown tag " << (int)Tag->first << " in message " << fmpe.msgId << std::endl;
        }
        return text;
	}
	case COLUMN_THREAD_AUTHOR: return QVariant();

	default:
		return QVariant("[ TODO ]");
	}


	return QVariant("[ERROR]");
}

QVariant RsMessageModel::userRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
	switch(col)
    {
     	case COLUMN_THREAD_AUTHOR:   return QVariant(QString::fromStdString(fmpe.srcId.toStdString()));
     	case COLUMN_THREAD_MSGID:    return QVariant(QString::fromStdString(fmpe.msgId));
    default:
        return QVariant();
    }
}

QVariant RsMessageModel::decorationRole(const Rs::Msgs::MsgInfoSummary& fmpe,int col) const
{
	if(col == COLUMN_THREAD_READ)
		if(fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER))
			return QIcon(":/images/message-state-unread.png");
		else
			return QIcon(":/images/message-state-read.png");

    if(col == COLUMN_THREAD_SUBJECT)
    {
        if(fmpe.msgflags & RS_MSG_NEW         )  return QIcon(":/images/message-state-new.png");
        if(fmpe.msgflags & RS_MSG_USER_REQUEST)  return QIcon(":/images/user/user_request16.png");
        if(fmpe.msgflags & RS_MSG_FRIEND_RECOMMENDATION) return QIcon(":/images/user/friend_suggestion16.png");
        if(fmpe.msgflags & RS_MSG_PUBLISH_KEY) return QIcon(":/images/share-icon-16.png");

        if(fmpe.msgflags & RS_MSG_UNREAD_BY_USER)
        {
            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED)    return QIcon(":/images/message-mail-replied.png");
            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED)  return QIcon(":/images/message-mail-forwarded.png");
            if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) return QIcon(":/images/message-mail-replied-forw.png");

            return QIcon(":/images/message-mail.png");
        }
		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED)    return QIcon(":/images/message-mail-replied-read.png");
		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED)  return QIcon(":/images/message-mail-forwarded-read.png");
		if((fmpe.msgflags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) return QIcon(":/images/message-mail-replied-forw-read.png");

		return QIcon(":/images/message-mail-read.png");
    }

    if(col == COLUMN_THREAD_STAR)
        return QIcon((fmpe.msgflags & RS_MSG_STAR) ? (IMAGE_STAR_ON ): (IMAGE_STAR_OFF));

    bool isNew = fmpe.msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);

    if(col == COLUMN_THREAD_READ)
        return QIcon(isNew ? ":/images/message-state-unread.png": ":/images/message-state-read.png");

	return QVariant();
}

void RsMessageModel::clear()
{
    preMods();

    mMessages.clear();

	postMods();

	emit messagesLoaded();
}

void RsMessageModel::setMessages(const std::list<Rs::Msgs::MsgInfoSummary>& msgs)
{
    preMods();

    beginRemoveRows(QModelIndex(),0,mMessages.size()-1);
    endRemoveRows();

    mMessages.clear();
    mMessagesMap.clear();

    for(auto it(msgs.begin());it!=msgs.end();++it)
    {
        mMessagesMap[(*it).msgId] = mMessages.size();
    	mMessages.push_back(*it);
    }

    // now update prow for all posts

#ifdef DEBUG_MESSAGE_MODEL
    debug_dump();
#endif

    beginInsertRows(QModelIndex(),0,mMessages.size()-1);
    endInsertRows();
	postMods();

	emit messagesLoaded();
}

void RsMessageModel::setCurrentBox(BoxName bn)
{
    if(mCurrentBox != bn)
    {
		mCurrentBox = bn;
        updateMessages();
    }
}

void RsMessageModel::setQuickViewFilter(QuickViewFilter fn)
{
    if(fn != mQuickViewFilter)
    {
        std::cerr << "Changing new quickview filter to " << fn << std::endl;

		preMods();
        mQuickViewFilter = fn ;
		postMods();
    }
}

void RsMessageModel::getMessageSummaries(BoxName box,std::list<Rs::Msgs::MsgInfoSummary>& msgs)
{
    rsMsgs->getMessageSummaries(msgs);

    // filter out messages that are not in the right box.

    for(auto it(msgs.begin());it!=msgs.end();)
    {
        bool ok = false;

        switch(box)
        {
		case BOX_INBOX  : ok = (it->msgflags & RS_MSG_BOXMASK) == RS_MSG_INBOX     && !(it->msgflags & RS_MSG_TRASH); break ;
        case BOX_SENT   : ok = (it->msgflags & RS_MSG_BOXMASK) == RS_MSG_SENTBOX   && !(it->msgflags & RS_MSG_TRASH); break ;
        case BOX_OUTBOX : ok = (it->msgflags & RS_MSG_BOXMASK) == RS_MSG_OUTBOX    && !(it->msgflags & RS_MSG_TRASH); break ;
        case BOX_DRAFTS : ok = (it->msgflags & RS_MSG_BOXMASK) == RS_MSG_DRAFTBOX  && !(it->msgflags & RS_MSG_TRASH); break ;
        case BOX_TRASH  : ok = (it->msgflags & RS_MSG_TRASH) ; break ;
        default:
            			++it;
                        continue;
		}

        if(ok)
            ++it;
		else
           it = msgs.erase(it) ;
    }
}

void RsMessageModel::updateMessages()
{
    emit messagesAboutToLoad();

    std::list<Rs::Msgs::MsgInfoSummary> msgs;

    getMessageSummaries(mCurrentBox,msgs);
	setMessages(msgs);

    emit messagesLoaded();
}

static bool decreasing_time_comp(const std::pair<time_t,RsGxsMessageId>& e1,const std::pair<time_t,RsGxsMessageId>& e2) { return e2.first < e1.first ; }

void RsMessageModel::setMsgReadStatus(const QModelIndex& i,bool read_status)
{
	if(!i.isValid())
		return ;

    preMods();
    rsMsgs->MessageRead(i.data(MsgIdRole).toString().toStdString(),!read_status);

    emit dataChanged(i.sibling(i.row(),0),i.sibling(i.row(),COLUMN_THREAD_NB_COLUMNS-1));
}

void RsMessageModel::setMsgStar(const QModelIndex& i,bool star)
{
    preMods();
    rsMsgs->MessageStar(i.data(MsgIdRole).toString().toStdString(),star);

    emit dataChanged(i.sibling(i.row(),0),i.sibling(i.row(),COLUMN_THREAD_NB_COLUMNS-1));
}

QModelIndex RsMessageModel::getIndexOfMessage(const std::string& mid) const
{
    // Brutal search. This is not so nice, so dont call that in a loop! If too costly, we'll use a map.

    auto it = mMessagesMap.find(mid);

    if(it == mMessagesMap.end() || it->second >= mMessages.size())
        return QModelIndex();

	quintptr ref ;
	convertMsgIndexToInternalId(it->second,ref);

	return createIndex(it->second,0,ref);
}

void RsMessageModel::debug_dump() const
{
    for(auto it(mMessages.begin());it!=mMessages.end();++it)
		std::cerr << "Id: " << it->msgId << ": from " << it->srcId << ": flags=" << it->msgflags << ": title=\"" << it->title << "\"" << std::endl;
}
