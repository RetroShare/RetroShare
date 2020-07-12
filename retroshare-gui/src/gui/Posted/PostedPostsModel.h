/*******************************************************************************
 * retroshare-gui/src/gui/gxschannels/GxsChannelPostsModel.h                   *
 *                                                                             *
 * Copyright 2020 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include "retroshare/rsposted.h"
#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rsevents.h"

#include <QModelIndex>
#include <QColor>

// This class holds the actual hierarchy of posts, represented by identifiers
// It is responsible for auto-updating when necessary and holds a mutex to allow the Model to
// safely access the data.

// The model contains a post in place 0 that is the parent of all posts.

// The model contains 3 layers:
//
//   Layer 1: list of all posts
//
//    * this list is sorted according to the current sorting strategy
//    * Variables: mPosts
//
//   Layer 2: list of post filtered by search
//
//    * depending on which chunk of posts are actually displayed, this list contains
//      the subset of the general list of posts
//    * Variables: mFilteredPosts
//
//   Layer 3: start and end of posts actually displayed in the previous list
//
//    * Variables: mDisplayedStartIndex, mDisplayedNbPosts
//
// The array below indicates which variables are updated depending on the type of data/view change:
//
//             |  Global list (mPosts) | Filtered List    |  Displayed list (mDisplayedStartIndex, mDisplayedNbPosts)
//  -----------+-----------------------+------------------+----------------------------------------------------------
//  New group  |           X           |        X         |  X (updated because FilteredList may change)
//  Sort order |           X           |                  |
//  Filter Str |                       |        X         |  X (updated because FilteredList may change)
//  Chunk chng |                       |        X         |  X
//
// In the model, indexes internal refs are pointer casts of the index in the mFilteredPosts tab. Another possible choice
// was to use indexes in the tab of displayed indices, but this leads to a more complex impleemntation.

typedef uint32_t PostedPostsModelIndex;

// struct ChannelPostsModelPostEntry
// {
//     ChannelPostsModelPostEntry() : mPublishTs(0),mPostFlags(0),mMsgStatus(0),prow(0) {}
//
//     enum {					// flags for display of posts. To be used in mPostFlags
//         FLAG_POST_IS_PINNED              = 0x0001,
//         FLAG_POST_IS_MISSING             = 0x0002,
//         FLAG_POST_IS_REDACTED            = 0x0004,
//         FLAG_POST_HAS_UNREAD_CHILDREN    = 0x0008,
//         FLAG_POST_HAS_READ_CHILDREN      = 0x0010,
//         FLAG_POST_PASSES_FILTER          = 0x0020,
//         FLAG_POST_CHILDREN_PASSES_FILTER = 0x0040,
//     };
//
//     std::string        mTitle ;
//     RsGxsMessageId     mMsgId;
//     uint32_t           mPublishTs;
//     uint32_t           mPostFlags;
//     int                mMsgStatus;
//
//     int prow ;// parent row, which basically means position in the array of posts
// };

// This class is the item model used by Qt to display the information

class RsPostedPostsModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsPostedPostsModel(QObject *parent = NULL);
	virtual ~RsPostedPostsModel() override;

	static const uint32_t COLUMN_THREAD_NB_COLUMNS   = 0x01;
	static const uint32_t DEFAULT_DISPLAYED_NB_POSTS ;

    enum SortingStrategy {
        SORT_UNKNOWN         = 0x00,
        SORT_NEW_SCORE       = 0x01,
        SORT_TOP_SCORE       = 0x02,
        SORT_HOT_SCORE       = 0x03
    };

	enum Columns {
		COLUMN_POSTS        =0x00,
		COLUMN_THREAD_MSGID =0x01,
		COLUMN_THREAD_DATA  =0x02,
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	StatusRole         = Qt::UserRole+2,
              	FilterRole         = Qt::UserRole+3,
              };

    enum TreeMode{ TREE_MODE_UNKWN  = 0x00,
                   TREE_MODE_PLAIN  = 0x01,
                   TREE_MODE_FILES  = 0x02,
    };

#ifdef TODO
    enum SortMode{ SORT_MODE_PUBLISH_TS           = 0x00,
                   SORT_MODE_CHILDREN_PUBLISH_TS  = 0x01,
    };
#endif

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
	QModelIndex getIndexOfMessage(const RsGxsMessageId& mid) const;

    // This method will asynchroneously update the data

	void updateBoard(const RsGxsGroupId& posted_group_id);
    const RsGxsGroupId& currentGroupId() const;

#ifdef TODO
    void setSortMode(SortMode mode) ;

	void setTextColorRead          (QColor color) { mTextColorRead           = color;}
	void setTextColorUnread        (QColor color) { mTextColorUnread         = color;}
	void setTextColorUnreadChildren(QColor color) { mTextColorUnreadChildren = color;}
	void setTextColorNotSubscribed (QColor color) { mTextColorNotSubscribed  = color;}
	void setTextColorMissing       (QColor color) { mTextColorMissing        = color;}
#endif

	void setMsgReadStatus(const QModelIndex &i, bool read_status, bool with_children);
    void setFilter(const QStringList &strings, uint32_t &count) ;
	void setSortingStrategy(SortingStrategy s);
	void setPostsInterval(int start,int nb_posts);

#ifdef TODO
	void setAuthorOpinion(const QModelIndex& indx,RsOpinion op);
#endif

    // Helper functions

    bool getPostData(const QModelIndex& i,RsPostedPost& fmpe) const ;
    uint32_t totalPostsCount() const { return mPosts.size() ; }
    uint32_t filteredPostsCount() const { return mFilteredPosts.size() ; }
    uint32_t displayedStartPostIndex() const { return mDisplayedStartIndex ; }
    void clear() ;

    // AbstractItemModel functions.

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // Custom item roles

    QVariant sizeHintRole  (int col) const;
	QVariant displayRole   (const RsPostedPost& fmpe, int col) const;
	QVariant toolTipRole   (const RsPostedPost& fmpe, int col) const;
	QVariant userRole      (const RsPostedPost& fmpe, int col) const;
#ifdef TODO
	QVariant decorationRole(const RsPostedPost& fmpe, int col) const;
	QVariant pinnedRole    (const RsPostedPost& fmpe, int col) const;
	QVariant missingRole   (const RsPostedPost& fmpe, int col) const;
	QVariant statusRole    (const RsPostedPost& fmpe, int col) const;
	QVariant authorRole    (const RsPostedPost& fmpe, int col) const;
	QVariant sortRole      (const RsPostedPost& fmpe, int col) const;
	QVariant fontRole      (const RsPostedPost& fmpe, int col) const;
	QVariant filterRole    (const RsPostedPost& fmpe, int col) const;
	QVariant textColorRole (const RsPostedPost& fmpe, int col) const;
	QVariant backgroundRole(const RsPostedPost& fmpe, int col) const;
#endif

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */
    void debug_dump();

signals:
    void boardPostsLoaded();	// emitted after the posts have been loaded.

private:
    RsPostedGroup mPostedGroup;

    TreeMode mTreeMode;

	void preMods() ;
	void postMods() ;

    quintptr getParentRow(quintptr ref,int& row) const;
    quintptr getChildRef(quintptr ref, int index) const;
    int      getChildrenCount(quintptr ref) const;

    static bool convertTabEntryToRefPointer(uint32_t entry, quintptr &ref);
	static bool convertRefPointerToTabEntry(quintptr ref,uint32_t& entry);
	static void computeReputationLevel(uint32_t forum_sign_flags, RsPostedPost& entry);

	void update_posts(const RsGxsGroupId& group_id);

#ifdef TODO
	void setForumMessageSummary(const std::vector<RsGxsForumMsg>& messages);
	void     recursUpdateReadStatusAndTimes(ChannelPostsModelIndex i,bool& has_unread_below,bool& has_read_below);
	uint32_t recursUpdateFilterStatus(ChannelPostsModelIndex i,int column,const QStringList& strings);
	void     recursSetMsgReadStatus(ChannelPostsModelIndex i,bool read_status,bool with_children);
#endif

	void createPostsArray(std::vector<RsPostedPost> &posts);
	void setPosts(const RsPostedGroup& group, std::vector<RsPostedPost> &posts);
	void initEmptyHierarchy();
	void handleEvent_main_thread(std::shared_ptr<const RsEvent> event);

    std::vector<RsPostedPost> mPosts ;
    std::vector<int> mFilteredPosts;
    uint32_t mDisplayedStartIndex;
    uint32_t mDisplayedNbPosts;


	RsEventsHandlerId_t mEventHandlerId ;
};
