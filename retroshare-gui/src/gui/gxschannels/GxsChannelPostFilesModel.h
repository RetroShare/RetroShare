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

#pragma once

#include "retroshare/rsfiles.h"
#include "retroshare/rsgxscommon.h"

#include <QModelIndex>
#include <QColor>

// This class holds the actual hierarchy of posts, represented by identifiers
// It is responsible for auto-updating when necessary and holds a mutex to allow the Model to
// safely access the data.

// The model contains a post in place 0 that is the parent of all posts.

typedef uint32_t ChannelPostFilesModelIndex;

class QTimer;

// This class contains the info for a file as well as additional info such as publication date

struct ChannelPostFileInfo: public RsGxsFile
{
    ChannelPostFileInfo(const RsGxsFile& gxs_file,rstime_t t)
    	: RsGxsFile(gxs_file),mPublishTime(t)
    {}

    ChannelPostFileInfo() : mPublishTime(0) {}

    rstime_t mPublishTime;
};

// This class is the item model used by Qt to display the information

class RsGxsChannelPostFilesModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsGxsChannelPostFilesModel(QObject *parent = NULL);
	~RsGxsChannelPostFilesModel(){}

	enum Columns {
		COLUMN_FILES_NAME        = 0x00,
		COLUMN_FILES_SIZE        = 0x01,
		COLUMN_FILES_FILE        = 0x02,
		COLUMN_FILES_DATE        = 0x03,
		COLUMN_FILES_NB_COLUMNS  = 0x04
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	FilterRole         = Qt::UserRole+2,
              };

#ifdef TODO
    enum SortMode{ SORT_MODE_PUBLISH_TS           = 0x00,
                   SORT_MODE_CHILDREN_PUBLISH_TS  = 0x01,
    };
#endif

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}

    // This method will asynchroneously update the data
	void setFiles(const std::list<ChannelPostFileInfo>& files);
    void setFilter(const QStringList &strings, uint32_t &count) ;

#ifdef TODO
	QModelIndex getIndexOfFile(const RsFileHash& hash) const;
    void setSortMode(SortMode mode) ;

	void setTextColorRead          (QColor color) { mTextColorRead           = color;}
	void setTextColorUnread        (QColor color) { mTextColorUnread         = color;}
	void setTextColorUnreadChildren(QColor color) { mTextColorUnreadChildren = color;}
	void setTextColorNotSubscribed (QColor color) { mTextColorNotSubscribed  = color;}
	void setTextColorMissing       (QColor color) { mTextColorMissing        = color;}
	void setAuthorOpinion(const QModelIndex& indx,RsOpinion op);
#endif

    // Helper functions

    void clear() ;

    // AbstractItemModel functions.

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    // Custom item roles

    QVariant sizeHintRole  (int col) const;
	QVariant displayRole   (const ChannelPostFileInfo& fmpe, int col) const;
	QVariant toolTipRole   (const ChannelPostFileInfo& fmpe, int col) const;
	QVariant userRole      (const ChannelPostFileInfo& fmpe, int col) const;
	QVariant sortRole      (const ChannelPostFileInfo& fmpe, int col) const;
	QVariant filterRole    (const ChannelPostFileInfo& fmpe, int col) const;
#ifdef TODO
	QVariant decorationRole(const ForumModelPostEntry& fmpe, int col) const;
	QVariant pinnedRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant missingRole   (const ForumModelPostEntry& fmpe, int col) const;
	QVariant statusRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant authorRole    (const ForumModelPostEntry& fmpe, int col) const;
	QVariant fontRole      (const ForumModelPostEntry& fmpe, int col) const;
	QVariant textColorRole (const ForumModelPostEntry& fmpe, int col) const;
	QVariant backgroundRole(const ForumModelPostEntry& fmpe, int col) const;
#endif

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */
    void debug_dump();

signals:
    void channelLoaded();	// emitted after the posts have been set. Can be used to updated the UI.

private slots:
    void update();

private:
#ifdef TODO
    bool mUseChildTS;
    bool mFilteringEnabled;
    SortMode mSortMode;
#endif
	void preMods() ;
	void postMods() ;

    quintptr getParentRow(quintptr ref,int& row) const;
    quintptr getChildRef(quintptr ref, int index) const;
    int   getChildrenCount(quintptr ref) const;
	bool getFileData(const QModelIndex& i, ChannelPostFileInfo &fmpe) const;

    static bool convertTabEntryToRefPointer(uint32_t entry, quintptr& ref);
	static bool convertRefPointerToTabEntry(quintptr ref,uint32_t& entry);

#ifdef TODO
	static void generateMissingItem(const RsGxsMessageId &msgId,ChannelPostsModelPostEntry& entry);
#endif
	void initEmptyHierarchy();

    std::vector<int> mFilteredFiles ;  // store the list of files for the post
    std::vector<ChannelPostFileInfo> mFiles ;  // store the list of files for the post

    QTimer *mTimer;
};
