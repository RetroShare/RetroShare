/*******************************************************************************
 * retroshare-gui/src/gui/msgs/RsMessageModel.h                                *
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

#pragma once

#include <QModelIndex>
#include <QColor>
#include <QFont>

#include "retroshare/rsmsgs.h"

// This class holds the actual hierarchy of posts, represented by identifiers
// It is responsible for auto-updating when necessary and holds a mutex to allow the Model to
// safely access the data.

// The model contains a post in place 0 that is the parent of all posts.

typedef uint32_t ForumModelIndex;

// This class is the item model used by Qt to display the information

class RsMessageModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsMessageModel(QObject *parent = NULL);
	~RsMessageModel(){}

	enum Columns {
		COLUMN_THREAD_STAR         = 0x00,
		COLUMN_THREAD_ATTACHMENT   = 0x01,
		COLUMN_THREAD_SUBJECT      = 0x02,
		COLUMN_THREAD_READ         = 0x03,
		COLUMN_THREAD_AUTHOR       = 0x04,
        COLUMN_THREAD_TO           = 0x05,
        COLUMN_THREAD_SPAM         = 0x06,
        COLUMN_THREAD_DATE         = 0x07,
        COLUMN_THREAD_TAGS         = 0x08,
        COLUMN_THREAD_MSGID        = 0x09,
        COLUMN_THREAD_CONTENT      = 0x0a,
        COLUMN_THREAD_NB_COLUMNS   = 0x0b
    };

    enum QuickViewFilter {
        QUICK_VIEW_ALL             = 0x00,
        QUICK_VIEW_IMPORTANT       = 0x01,	// These numbers have been carefuly chosen to match the ones in rsmsgs.h
        QUICK_VIEW_WORK            = 0x02,
        QUICK_VIEW_PERSONAL        = 0x03,
        QUICK_VIEW_TODO            = 0x04,
        QUICK_VIEW_LATER           = 0x05,
        QUICK_VIEW_STARRED         = 0x06,
        QUICK_VIEW_SYSTEM          = 0x07,
        QUICK_VIEW_SPAM            = 0x08,
        QUICK_VIEW_ATTACHMENT      = 0x09,
        QUICK_VIEW_USER            = 100
    };

    enum FilterType {
        FILTER_TYPE_NONE             = 0x00,
        FILTER_TYPE_SUBJECT          = 0x01,	// These numbers have been carefuly chosen to match the ones in rsmsgs.h
        FILTER_TYPE_FROM             = 0x02,
        FILTER_TYPE_TO               = 0x03,
        FILTER_TYPE_DATE             = 0x04,
        FILTER_TYPE_CONTENT          = 0x05,
        FILTER_TYPE_TAGS             = 0x06,
        FILTER_TYPE_ATTACHMENTS      = 0x07,
    };

	enum Roles{ SortRole           = Qt::UserRole+1,
              	StatusRole         = Qt::UserRole+2,
              	UnreadRole         = Qt::UserRole+3,
              	FilterRole         = Qt::UserRole+4,
              	MsgIdRole          = Qt::UserRole+5,
              	MsgFlagsRole       = Qt::UserRole+6,
              	SrcIdRole          = Qt::UserRole+7,
              };

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
	QModelIndex getIndexOfMessage(const std::string &mid) const;

    static const QString FilterString ;

    // This method will asynchroneously update the data

    void setCurrentBox(Rs::Msgs::BoxName bn) ;
    Rs::Msgs::BoxName currentBox() const ;
    void setQuickViewFilter(QuickViewFilter fn) ;

    void setFilter(FilterType filter_type, const QStringList& strings) ;
    void setFont(const QFont &font);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	bool getMessageData(const QModelIndex& i,Rs::Msgs::MessageInfo& fmpe) const;
    void clear() ;

    QVariant sizeHintRole  (int col) const;

	QVariant displayRole   (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant decorationRole(const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant toolTipRole   (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant userRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant statusRole    (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant authorRole    (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant sortRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant fontRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant filterRole    (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant textColorRole (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant backgroundRole(const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;

#ifdef DEBUG_MESSAGE_MODEL
	/*!
	 * \brief debug_dump
	 * Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
	 */
	void debug_dump() const;
#endif

	// control over message flags and so on. This is handled by the model because it will allow it to update accordingly
	void setMsgReadStatus(const QModelIndex& i, bool read_status);
	void setMsgsReadStatus(const QModelIndexList& mil, bool read_status);
	void setMsgStar(const QModelIndex& i,bool star) ;
	void setMsgsStar(const QModelIndexList& mil,bool star) ;
	void setMsgJunk(const QModelIndex& i,bool junk) ;
	void setMsgsJunk(const QModelIndexList& mil,bool junk) ;

public slots:
	void updateMessages();

signals:
    void messagesLoaded();	// emitted after the messages have been set. Can be used to updated the UI.
    void messagesAboutToLoad();

private:
	bool passesFilter(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    //bool hasIndex(int row,int column,const QModelIndex& parent)const;
    int  getChildrenCount(void *ref) const;

    static bool convertMsgIndexToInternalId(uint32_t entry,quintptr& ref);
	static bool convertInternalIdToMsgIndex(quintptr ref,uint32_t& index);

	uint32_t updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);

	void setMessages(const std::list<Rs::Msgs::MsgInfoSummary>& msgs);

    QColor mTextColorRead          ;
    QColor mTextColorUnread        ;
    QColor mTextColorUnreadChildren;
    QColor mTextColorNotSubscribed ;
    QColor mTextColorMissing       ;

    Rs::Msgs::BoxName mCurrentBox ;
    QuickViewFilter mQuickViewFilter ;
    QStringList mFilterStrings;
    FilterType  mFilterType;
    QFont mFont;

    std::vector<Rs::Msgs::MsgInfoSummary> mMessages;
    std::map<std::string,uint32_t> mMessagesMap;
};
