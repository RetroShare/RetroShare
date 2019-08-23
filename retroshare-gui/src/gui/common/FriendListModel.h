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

typedef uint32_t ForumModelIndex;

// This class is the item model used by Qt to display the information

class RsFriendListModel : public QAbstractItemModel
{
	Q_OBJECT

public:
	explicit RsFriendListModel(QObject *parent = NULL);
	~RsFriendListModel(){}

    class RsNodeDetails: public RsPeerDetails {};// in the near future, there will be a specific class for Profile/Node details in replacement of RsPeerDetails
    class RsProfileDetails: public RsPeerDetails {};

	struct HierarchicalGroupInformation
	{
        RsGroupInfo group_info;
		std::vector<uint32_t> child_profile_indices;	// index in the array of hierarchical profiles
	};
	struct HierarchicalProfileInformation
	{
        RsProfileDetails profile_info;
		std::vector<uint32_t> child_node_indices;		// indices of nodes in the array of nodes.
	};
	struct HierarchicalNodeInformation
	{
        HierarchicalNodeInformation() : last_update_ts(0) {}

        rstime_t last_update_ts;
        RsNodeDetails node_info;
	};

	enum Columns {
		COLUMN_THREAD_NAME         = 0x00,
		COLUMN_THREAD_LAST_CONTACT = 0x01,
		COLUMN_THREAD_IP           = 0x02,
		COLUMN_THREAD_ID           = 0x03,
		COLUMN_THREAD_NB_COLUMNS   = 0x04
	};

	enum Roles{ SortRole           = Qt::UserRole+1,
              	StatusRole         = Qt::UserRole+2,
              	UnreadRole         = Qt::UserRole+3,
              	FilterRole         = Qt::UserRole+4,
              	OnlineRole         = Qt::UserRole+5
              };

    enum FilterType{ FILTER_TYPE_NONE = 0x00,
        			 FILTER_TYPE_ID   = 0x01,
                     FILTER_TYPE_NAME = 0x02
                   };

    enum EntryType{ ENTRY_TYPE_UNKNOWN   = 0x00,
                    ENTRY_TYPE_GROUP     = 0x01,
                    ENTRY_TYPE_PROFILE   = 0x02,
                    ENTRY_TYPE_NODE      = 0x03
                  };

    // This structure encodes the position of a node in the hierarchy. The type tells which of the index fields are valid.

    struct EntryIndex
    {
    public:
        EntryIndex() : type(ENTRY_TYPE_UNKNOWN),group_index(0xff),profile_index(0xff),node_index(0xff) {}

        EntryType type;		        // type of the entry (group,profile,location)

        // Indices w.r.t. parent. The set of indices entirely determines the position of the entry in the hierarchy.
        // An index of 0xff means "undefined"

        uint8_t group_index;		// index of the group in mGroups tab
        uint8_t profile_index;		// index of the child profile in its own group if group_index < 0xff, or in the mProfiles tab otherwise.
        uint8_t node_index;			// index of the child node in its own profile

        EntryIndex parent() const;
		EntryIndex child(int row,const std::vector<EntryIndex>& top_level) const;
        uint32_t   parentRow(uint32_t nb_groups) const;
    };

	QModelIndex root() const{ return createIndex(0,0,(void*)NULL) ;}
	QModelIndex getIndexOfGroup(const RsNodeGroupId& mid) const;

    static const QString FilterString ;

    // This method will asynchroneously update the data

	void setDisplayGroups(bool b);
    bool getDisplayGroups() const { return mDisplayGroups; }

	void setDisplayStatusString(bool b);
    bool getDisplayStatusString() const { return mDisplayStatusString; }

    EntryType getType(const QModelIndex&) const;

    bool getGroupData  (const QModelIndex&,RsGroupInfo     &) const;
    bool getProfileData(const QModelIndex&,RsProfileDetails&) const;
    bool getNodeData   (const QModelIndex&,RsNodeDetails   &) const;

    void setFilter(FilterType filter_type, const QStringList& strings) ;

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

	/* Color definitions (for standard see qss.default) */
	QColor mTextColorGroup;
	QColor mTextColorStatus[RS_STATUS_COUNT];

private:
    const HierarchicalGroupInformation   *getGroupInfo  (const EntryIndex&) const;
    const HierarchicalProfileInformation *getProfileInfo(const EntryIndex&)const;
    const HierarchicalNodeInformation    *getNodeInfo(const EntryIndex&) const;

	void checkNode(HierarchicalNodeInformation& node);
	bool getPeerOnlineStatus(const EntryIndex& e) const;
	std::map<RsPgpId,uint32_t>::const_iterator checkProfileIndex(const RsPgpId& pgp_id,
                                                                 std::map<RsPgpId,uint32_t>& pgp_indices,
                                                                 std::vector<HierarchicalProfileInformation>& mProfiles,
                                                                 bool create);

    QVariant sizeHintRole  (const EntryIndex& e, int col) const;
	QVariant displayRole   (const EntryIndex& e, int col) const;
	QVariant decorationRole(const EntryIndex& e, int col) const;
	QVariant toolTipRole   (const EntryIndex& e, int col) const;
	QVariant statusRole    (const EntryIndex& e, int col) const;
	QVariant sortRole      (const EntryIndex& e, int col) const;
	QVariant fontRole      (const EntryIndex& e, int col) const;
	QVariant textColorRole (const EntryIndex& e, int col) const;
	QVariant onlineRole    (const EntryIndex& e, int col) const;
	QVariant filterRole    (const EntryIndex& e, int col) const;

    /*!
     * \brief debug_dump
     * 			Dumps the hierarchy of posts in the terminal, to allow checking whether the internal representation is correct.
     */

public slots:
	void checkInternalData(bool force);
    void debug_dump() const;

signals:
    void dataLoaded();	// emitted after the messages have been set. Can be used to updated the UI.
    void friendListChanged();	// emitted after the messages have been set. Can be used to updated the UI.
    void dataAboutToLoad();

private:
	void updateInternalData();
	bool passesFilter(const EntryIndex &e, int column) const;

	void preMods() ;
	void postMods() ;

    void *getParentRef(void *ref,int& row) const;
    void *getChildRef(void *ref,int row) const;
    int  getChildrenCount(void *ref) const;

    template<uint8_t> static bool convertIndexToInternalId(const EntryIndex& e,quintptr& ref);
	template<uint8_t> static bool convertInternalIdToIndex(quintptr ref, EntryIndex& e);

	uint32_t updateFilterStatus(ForumModelIndex i,int column,const QStringList& strings);

    QStringList mFilterStrings;
    FilterType  mFilterType;

    bool mDisplayGroups ;
    bool mDisplayStatusString ;
    rstime_t mLastInternalDataUpdate;
    rstime_t mLastNodeUpdate;;

    // The 3 vectors below store thehierarchical information for groups, profiles and locations,
    // meaning which is the child/parent of which. The actual group/profile/node data are also stored in the
    // structure.

    mutable std::vector<HierarchicalGroupInformation>   mGroups;
    mutable std::vector<HierarchicalProfileInformation> mProfiles;
    mutable std::vector<HierarchicalNodeInformation>    mLocations;

    // The top level list contains all nodes to display, which type depends on the option to display groups or not.
    // Idices in the list may be profile indices or group indices. In the former case the profile child index refers to
    // the inde in the mProfiles tab, whereas in the the later case, the child index refers to the index of the profile in the
    // group it belows to.

    std::vector<EntryIndex> mTopLevel;
};

