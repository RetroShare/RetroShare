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

#include "retroshare/rsmsgs.h"

// This class holds the actual hierarchy of posts, represented by identifiers
// It is responsible for auto-updating when necessary and holds a mutex to allow the Model to
// safely access the data.

// The model contains a post in place 0 that is the parent of all posts.

typedef uint32_t ForumModelIndex;

struct ForumModelPostEntry
{
    ForumModelPostEntry() : mPublishTs(0),mMostRecentTsInThread(0),mPostFlags(0),mReputationWarningLevel(0),mMsgStatus(0),prow(0) {}

    enum {					// flags for display of posts. To be used in mPostFlags
        FLAG_POST_IS_PINNED              = 0x0001,
        FLAG_POST_IS_MISSING             = 0x0002,
        FLAG_POST_IS_REDACTED            = 0x0004,
        FLAG_POST_HAS_UNREAD_CHILDREN    = 0x0008,
        FLAG_POST_HAS_READ_CHILDREN      = 0x0010,
        FLAG_POST_PASSES_FILTER          = 0x0020,
        FLAG_POST_CHILDREN_PASSES_FILTER = 0x0040,
    };

    std::string        mTitle ;
    RsGxsId            mAuthorId ;
    RsGxsMessageId     mMsgId;
    uint32_t           mPublishTs;
    uint32_t           mMostRecentTsInThread;
    uint32_t           mPostFlags;
    int                mReputationWarningLevel;
    int                mMsgStatus;

    std::vector<ForumModelIndex> mChildren;
    ForumModelIndex mParent;
    int prow ;									// parent row
};

// This class is the item model used by Qt to display the information

class RsMessageModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsMessageModel(QObject *parent = NULL);
	~RsMessageModel(){}

	enum Columns {
		COLUMN_THREAD_STAR         =0x00,
		COLUMN_THREAD_ATTACHMENT   =0x01,
		COLUMN_THREAD_SUBJECT      =0x02,
		COLUMN_THREAD_READ         =0x03,
		COLUMN_THREAD_AUTHOR       =0x04,
		COLUMN_THREAD_DATE         =0x05,
		COLUMN_THREAD_TAGS         =0x06,
		COLUMN_THREAD_MSGID        =0x07,
		COLUMN_THREAD_NB_COLUMNS   =0x08,
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	StatusRole         = Qt::UserRole+2,
              	UnreadRole         = Qt::UserRole+3,
              	FilterRole         = Qt::UserRole+4,
              };

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
	QModelIndex getIndexOfMessage(const std::string &mid) const;

    static const QString FilterString ;

    // This method will asynchroneously update the data

	void updateMessages();
    const RsMessageId& currentMessageId() const;

	void setMsgReadStatus(const QModelIndex& i, bool read_status);
    void setFilter(int column, const QStringList& strings, uint32_t &count) ;

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

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */
    void debug_dump() const;

signals:
    void messagesLoaded();	// emitted after the messages have been set. Can be used to updated the UI.

private:
    bool mFilteringEnabled;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    //bool hasIndex(int row,int column,const QModelIndex& parent)const;
    int  getChildrenCount(void *ref) const;

    static bool convertMsgIndexToInternalId(uint32_t entry,quintptr& ref);
	static bool convertInternalIdToMsgIndex(quintptr ref,uint32_t& index);
	static void computeReputationLevel(uint32_t forum_sign_flags, ForumModelPostEntry& entry);

	void update_posts(const RsGxsGroupId &group_id);
	uint32_t updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);

	static void generateMissingItem(const RsGxsMessageId &msgId,ForumModelPostEntry& entry);
	static ForumModelIndex addEntry(std::vector<ForumModelPostEntry>& posts,const ForumModelPostEntry& entry,ForumModelIndex parent);

	void setMessages(const std::list<Rs::Msgs::MsgInfoSummary>& msgs);

    QColor mTextColorRead          ;
    QColor mTextColorUnread        ;
    QColor mTextColorUnreadChildren;
    QColor mTextColorNotSubscribed ;
    QColor mTextColorMissing       ;

    std::vector<Rs::Msgs::MsgInfoSummary> mMessages;
};
