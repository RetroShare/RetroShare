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

#include "retroshare/rsstatus.h"
#include "retroshare/rsmsgs.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsidentity.h"

typedef uint32_t ForumModelIndex;

// This class is the item model used by Qt to display the information

class QTimer;

class RsIdentityListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
    explicit RsIdentityListModel(QObject *parent = NULL);
    ~RsIdentityListModel(){}

    enum Columns {
		COLUMN_THREAD_NAME         = 0x00,
        COLUMN_THREAD_ID           = 0x01,
        COLUMN_THREAD_OWNER_NAME   = 0x02,
        COLUMN_THREAD_OWNER_ID     = 0x03,
        COLUMN_THREAD_REPUTATION   = 0x04,
        COLUMN_THREAD_NB_COLUMNS   = 0x05
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	StatusRole         = Qt::UserRole+2,
              	UnreadRole         = Qt::UserRole+3,
              	FilterRole         = Qt::UserRole+4,
                TreePathRole       = Qt::UserRole+5,
              };

    enum FilterType{ FILTER_TYPE_NONE       = 0x00,
                     FILTER_TYPE_ID         = 0x01,
                     FILTER_TYPE_NAME       = 0x02,
                     FILTER_TYPE_OWNER_NAME = 0x04,
                     FILTER_TYPE_OWNER_ID   = 0x08
                   };

    enum EntryType{ ENTRY_TYPE_TOP_LEVEL = 0x00,
                    ENTRY_TYPE_CATEGORY  = 0x01,
                    ENTRY_TYPE_IDENTITY  = 0x02,
                    ENTRY_TYPE_INVALID   = 0x03
                  };

    enum Category{ 	CATEGORY_OWN = 0x00,
                    CATEGORY_CTS = 0x01,
                    CATEGORY_ALL = 0x02
                 };

    struct HierarchicalCategoryInformation
    {
        QString category_name;
        std::vector<uint32_t> child_identity_indices;	// index in the array of hierarchical profiles
    };

    // This stores all the info that is useful avoiding a call to the more expensive getIdDetails()
    //
    struct HierarchicalIdentityInformation
    {
        rstime_t last_update_TS;
        RsGxsId id;
        RsPgpId owner;
        uint32_t flags;
        std::string nickname;
    };

    // This structure encodes the position of a node in the hierarchy. The type tells which of the index fields are valid.

    struct EntryIndex
    {
    public:
        EntryIndex();

        EntryType type;		        // type of the entry (group,profile,location)

        friend std::ostream& operator<<(std::ostream& o, const EntryIndex& e)
        {
            o << "[" ;
            switch(e.type)
            {
            case RsIdentityListModel::ENTRY_TYPE_INVALID: o << "Invalid," ; break;
            case RsIdentityListModel::ENTRY_TYPE_CATEGORY: o << "Category," ; break;
            case RsIdentityListModel::ENTRY_TYPE_IDENTITY: o << "Identity," ; break;
            case RsIdentityListModel::ENTRY_TYPE_TOP_LEVEL: o << "Toplevel," ; break;
            }
            o << " CI: " << e.category_index << ", ";
            o << " II: " << e.identity_index << "]";
            return o;
        }

        // Indices w.r.t. parent. The set of indices entirely determines the position of the entry in the hierarchy.
        // An index of 0xff means "undefined"

        uint16_t category_index;	// index of the category in the mCategory array
        uint16_t identity_index;	// index of the identity in its own category

        EntryIndex parent() const;
        EntryIndex child(int row) const;
        uint32_t   parentRow() const;
    };

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
    QModelIndex getIndexOfIdentity(const RsGxsId& id) const;
    QModelIndex getIndexOfCategory(Category id) const;

    void updateIdentityList();

    int count() const { return mIdentities.size() ; }	// total number of identities

    static const QString FilterString ;

    // This method will asynchroneously update the data

    EntryType getType(const QModelIndex&) const;
    RsGxsId getIdentity(const QModelIndex&) const;
    int getCategory(const QModelIndex&) const;
    void setFontSize(int s);

    void setFilter(uint8_t filter_type, const QStringList& strings) ;

    void expandItem(const QModelIndex&) ;
    void collapseItem(const QModelIndex&) ;

    // Overloaded methods from QAbstractItemModel

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const  override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	void clear() ;
    QString indexIdentifier(QModelIndex i);

	/* Color definitions (for standard see default.qss) */
	QColor mTextColorGroup;
	QColor mTextColorStatus[RS_STATUS_COUNT];

    void setIdentities(const std::list<RsGroupMetaData>& identities_meta);

private:
    const HierarchicalCategoryInformation    *getCategoryInfo  (const EntryIndex&) const;
    const HierarchicalIdentityInformation    *getIdentityInfo(const EntryIndex&) const;

    void checkIdentity(HierarchicalIdentityInformation& node);

    QVariant sizeHintRole  (const EntryIndex& e, int col) const;
	QVariant displayRole   (const EntryIndex& e, int col) const;
	QVariant decorationRole(const EntryIndex& e, int col) const;
	QVariant toolTipRole   (const EntryIndex& e, int col) const;
	QVariant statusRole    (const EntryIndex& e, int col) const;
	QVariant sortRole      (const EntryIndex& e, int col) const;
	QVariant fontRole      (const EntryIndex& e, int col) const;
    QVariant foregroundRole(const EntryIndex& e, int col) const;
    QVariant textColorRole (const EntryIndex& e, int col) const;
	QVariant filterRole    (const EntryIndex& e, int col) const;
    QVariant treePathRole  (const EntryIndex& entry,int column) const;

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */

public slots:
	void checkInternalData(bool force);
    void debug_dump() const;
    void timerUpdate();

signals:
    void dataLoaded();	// emitted after the messages have been set. Can be used to updated the UI.
    void friendListChanged();	// emitted after the messages have been set. Can be used to updated the UI.
    void dataAboutToLoad();

private:
    bool passesFilter(const EntryIndex &e, int column) const;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    int  getChildrenCount(void *ref) const;

    static bool convertIndexToInternalId(const EntryIndex& e,quintptr& ref);
    static bool convertInternalIdToIndex(quintptr ref, EntryIndex& e);

	uint32_t updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);

    QStringList mFilterStrings;
    uint8_t  mFilterType;
    int mFontSize;

    rstime_t mLastInternalDataUpdate;
    rstime_t mLastNodeUpdate;;

    // The 3 vectors below store thehierarchical information for groups, profiles and locations,
    // meaning which is the child/parent of which. The actual group/profile/node data are also stored in the
    // structure.

    mutable std::vector<HierarchicalCategoryInformation>   mCategories;
    mutable std::vector<HierarchicalIdentityInformation>   mIdentities;

    // The top level list contains all nodes to display, which type depends on the option to display groups or not.
    // Idices in the list may be profile indices or group indices. In the former case the profile child index refers to
    // the inde in the mProfiles tab, whereas in the the later case, the child index refers to the index of the profile in the
    // group it belows to.

    std::vector<EntryIndex> mTopLevel;

    // keeps track of expanded/collapsed items, so as to only show icon for collapsed profiles

    std::vector<bool> mExpandedCategories;

    // List of identities for which getIdDetails() failed, to be requested again.
    mutable QTimer *mIdentityUpdateTimer;
};


