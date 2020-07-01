/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumModel.h                            *
 *                                                                             *
 * Copyright 2018 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include "retroshare/rsgxsforums.h"
#include "retroshare/rsgxsifacetypes.h"
#include <QModelIndex>
#include <QColor>

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

class RsGxsForumModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsGxsForumModel(QObject *parent = NULL);
	~RsGxsForumModel(){}

	enum Columns {
		COLUMN_THREAD_TITLE        =0x00,
		COLUMN_THREAD_READ         =0x01,
		COLUMN_THREAD_DATE         =0x02,
		COLUMN_THREAD_DISTRIBUTION =0x03,
		COLUMN_THREAD_AUTHOR       =0x04,
		COLUMN_THREAD_CONTENT      =0x05,
		COLUMN_THREAD_MSGID        =0x06,
		COLUMN_THREAD_DATA         =0x07,
		COLUMN_THREAD_NB_COLUMNS   =0x08,
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	ThreadPinnedRole   = Qt::UserRole+2,
              	MissingRole        = Qt::UserRole+3,
              	StatusRole         = Qt::UserRole+4,
              	UnreadChildrenRole = Qt::UserRole+5,
              	FilterRole         = Qt::UserRole+6,
              };

    enum TreeMode{ TREE_MODE_FLAT  = 0x00,
                   TREE_MODE_TREE  = 0x01,
    };

    enum SortMode{ SORT_MODE_PUBLISH_TS           = 0x00,
                   SORT_MODE_CHILDREN_PUBLISH_TS  = 0x01,
    };

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
	QModelIndex getIndexOfMessage(const RsGxsMessageId& mid) const;

    static const QString FilterString ;

	std::vector<std::pair<time_t,RsGxsMessageId> > getPostVersions(const RsGxsMessageId& mid) const;

    // This method will asynchroneously update the data
	void updateForum(const RsGxsGroupId& forumGroup);
    const RsGxsGroupId& currentGroupId() const;

    void setTreeMode(TreeMode mode) ;
    void setSortMode(SortMode mode) ;

	void setTextColorRead          (QColor color) { mTextColorRead           = color;}
	void setTextColorUnread        (QColor color) { mTextColorUnread         = color;}
	void setTextColorUnreadChildren(QColor color) { mTextColorUnreadChildren = color;}
	void setTextColorNotSubscribed (QColor color) { mTextColorNotSubscribed  = color;}
	void setTextColorMissing       (QColor color) { mTextColorMissing        = color;}
	void setTextColorPinned        (QColor color) { mTextColorPinned         = color;}

	void setMsgReadStatus(const QModelIndex &i, bool read_status, bool with_children);
    void setFilter(int column, const QStringList &strings, uint32_t &count) ;
	void setAuthorOpinion(const QModelIndex& indx,RsOpinion op);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    bool getPostData(const QModelIndex& i,ForumModelPostEntry& fmpe) const ;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    void clear() ;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant sizeHintRole  (int col) const;
	QVariant displayRole   (const ForumModelPostEntry& fmpe, int col) const;
	QVariant decorationRole(const ForumModelPostEntry& fmpe, int col) const;
	QVariant toolTipRole   (const ForumModelPostEntry& fmpe, int col) const;
	QVariant userRole      (const ForumModelPostEntry& fmpe, int col) const;
	QVariant pinnedRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant missingRole   (const ForumModelPostEntry& fmpe, int col) const;
	QVariant statusRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant authorRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant sortRole      (const ForumModelPostEntry& fmpe, int col) const;
	QVariant fontRole      (const ForumModelPostEntry& fmpe, int col) const;
	QVariant filterRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant textColorRole (const ForumModelPostEntry& fmpe, int col) const;
	QVariant backgroundRole(const ForumModelPostEntry& fmpe, int col) const;

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */
    void debug_dump();

signals:
    void forumLoaded();	// emitted after the posts have been set. Can be used to updated the UI.

private:
    RsGxsForumGroup mForumGroup;

    bool mUseChildTS;
    bool mFilteringEnabled;
    TreeMode mTreeMode;
    SortMode mSortMode;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    //bool hasIndex(int row,int column,const QModelIndex& parent)const;
    int  getChildrenCount(void *ref) const;

    static bool convertTabEntryToRefPointer(uint32_t entry,void *& ref);
	static bool convertRefPointerToTabEntry(void *ref,uint32_t& entry);
	static void computeReputationLevel(uint32_t forum_sign_flags, ForumModelPostEntry& entry);

	void update_posts(const RsGxsGroupId &group_id);
	void setForumMessageSummary(const std::vector<RsGxsForumMsg>& messages);
	void recursUpdateReadStatusAndTimes(ForumModelIndex i,bool& has_unread_below,bool& has_read_below);
	uint32_t recursUpdateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);
	void recursSetMsgReadStatus(ForumModelIndex i,bool read_status,bool with_children);

	static void generateMissingItem(const RsGxsMessageId &msgId,ForumModelPostEntry& entry);
	static ForumModelIndex addEntry(std::vector<ForumModelPostEntry>& posts,const ForumModelPostEntry& entry,ForumModelIndex parent);
	static void convertMsgToPostEntry(const RsGxsForumGroup &mForumGroup, const RsMsgMetaData &msg, bool useChildTS, ForumModelPostEntry& fentry);

	void computeMessagesHierarchy(const RsGxsForumGroup& forum_group, const std::vector<RsMsgMetaData> &msgs_array, std::vector<ForumModelPostEntry>& posts, std::map<RsGxsMessageId, std::vector<std::pair<time_t, RsGxsMessageId> > > &mPostVersions);
	void setPosts(const RsGxsForumGroup& group, const std::vector<ForumModelPostEntry>& posts,const std::map<RsGxsMessageId,std::vector<std::pair<time_t,RsGxsMessageId> > >& post_versions);
	void initEmptyHierarchy(std::vector<ForumModelPostEntry>& posts);

    std::vector<ForumModelPostEntry> mPosts ; // store the list of posts updated from rsForums.
	std::map<RsGxsMessageId,std::vector<std::pair<time_t,RsGxsMessageId> > > mPostVersions;

    QColor mTextColorRead          ;
    QColor mTextColorUnread        ;
    QColor mTextColorUnreadChildren;
    QColor mTextColorNotSubscribed ;
    QColor mTextColorMissing       ;
    QColor mTextColorPinned        ;

    friend class const_iterator;
};
