/*******************************************************************************
 * retroshare-gui/src/gui/msgs/RsFriendListModel.h                             *
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

// This class is the item model used by Qt to display the information

class RsFriendListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsFriendListModel(QObject *parent = NULL);
	~RsFriendListModel(){}

	enum Columns {
		COLUMN_THREAD_STAR         = 0x00,
		COLUMN_THREAD_ATTACHMENT   = 0x01,
		COLUMN_THREAD_SUBJECT      = 0x02,
		COLUMN_THREAD_READ         = 0x03,
		COLUMN_THREAD_AUTHOR       = 0x04,
		COLUMN_THREAD_DATE         = 0x05,
		COLUMN_THREAD_TAGS         = 0x06,
		COLUMN_THREAD_MSGID        = 0x07,
		COLUMN_THREAD_NB_COLUMNS   = 0x08,
		COLUMN_THREAD_CONTENT      = 0x08,
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

    const RsMessageId& currentMessageId() const;
    void setFilter(FilterType filter_type, const QStringList& strings) ;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	void clear() ;

    QVariant sizeHintRole  (int col) const;

	QVariant displayRole   (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant decorationRole(const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant toolTipRole   (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant userRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant statusRole    (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant sortRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant fontRole      (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant filterRole    (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;
	QVariant textColorRole (const Rs::Msgs::MsgInfoSummary& fmpe, int col) const;

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */
    void debug_dump() const;

public slots:
	void updateInternalData();

signals:
    void dataLoaded();	// emitted after the messages have been set. Can be used to updated the UI.
    void dataAboutToLoad();

private:
	bool passesFilter(const Rs::Msgs::MsgInfoSummary& fmpe,int column) const;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    int  getChildrenCount(void *ref) const;

    static bool convertMsgIndexToInternalId(uint32_t entry,quintptr& ref);
	static bool convertInternalIdToMsgIndex(quintptr ref,uint32_t& index);

	uint32_t updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);

    QStringList mFilterStrings;
    FilterType  mFilterType;

    std::vector<RsGroupInfo> mGroups;
    std::vector<RsPeerDetails> mProfiles;	// normally here we should have a specific class rather than RsPeerDetails
    std::vector<RsPeerDetails> mLocations;// normally here we should have a specific class rather than RsPeerDetails
};
