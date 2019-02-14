/*******************************************************************************
 * gui/RemoteDirModel.h                                                        *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
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

#ifndef REMOTE_DIR_MODEL
#define REMOTE_DIR_MODEL

#include <retroshare/rstypes.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QIcon>
#include <QMenu>

#include <stdint.h>
#include <vector>

#define COLUMN_NAME          0
#define COLUMN_FILENB        1
#define COLUMN_SIZE          2
#define COLUMN_AGE           3
#define COLUMN_FRIEND_ACCESS 4
#define COLUMN_WN_VISU_DIR   5
#define COLUMN_COUNT         6
#define RETROSHARE_DIR_MODEL_FILTER_STRING "filtered"

struct DirDetails;

class DirDetailsVector : public DirDetails
{
public:
	DirDetailsVector() : DirDetails() {}

	std::vector<DirStub> childrenVector; // For fast access with index (can we change the std::list on DirDetails to std::vector?)
};

static const uint32_t IND_LAST_DAY    = 3600*24 ;
static const uint32_t IND_LAST_WEEK   = 3600*24*7 ;
static const uint32_t IND_LAST_MONTH  = 3600*24*31	; // I know, this is approximate
static const uint32_t IND_ALWAYS      = ~(uint32_t)0 ;

class RetroshareDirModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
		enum Roles{ FileNameRole = Qt::UserRole+1, SortRole = Qt::UserRole+2, FilterRole = Qt::UserRole+3 };

		RetroshareDirModel(bool mode, QObject *parent = 0);
		virtual ~RetroshareDirModel() {}

		virtual Qt::ItemFlags flags ( const QModelIndex & index ) const;

		/* Callback from Core */
		virtual void preMods();
		virtual void postMods();

		void setVisible(bool b) { _visible = b ; }
		bool visible() { return _visible ;}

		/* Callback from GUI */
		void downloadSelected(const QModelIndexList &list, bool interactive);
		void createCollectionFile(QWidget *parent, const QModelIndexList &list);

		void getDirDetailsFromSelect (const QModelIndexList &list, std::vector <DirDetails>& dirVec);

		int getType ( const QModelIndex & index ) const ;
		void getFileInfoFromIndexList(const QModelIndexList& list, std::list<DirDetails>& files_info) ;
		void openSelected(const QModelIndexList &list);
		void getFilePaths(const QModelIndexList &list, std::list<std::string> &fullpaths);
		void getFilePath(const QModelIndex& index, std::string& fullpath);
		void changeAgeIndicator(uint32_t indicator) { ageIndicator = indicator; }

		bool requestDirDetails(void *ref, bool remote,DirDetails& d) const;

		virtual void update() {}
		virtual void updateRef(const QModelIndex&) const =0;

		virtual QMenu* getContextMenu(QMenu* contextMenu) {return contextMenu;}

		void filterItems(const std::list<std::string>& keywords, uint32_t& found) ;

		//Overloaded from QAbstractItemModel
		virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
		virtual QStringList mimeTypes () const;
		virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
		virtual Qt::DropActions supportedDragActions() const;
#endif

	protected:
		bool _visible ;

		void treeStyle();
		void downloadDirectory(const DirDetails & details, int prefixLen);
		static QString getFlagsString(FileStorageFlags f) ;
		static QString getGroupsString(FileStorageFlags flags, const std::list<RsNodeGroupId> &) ;
		QString getAgeIndicatorString(const DirDetails &) const;
//		void getAgeIndicatorRec(const DirDetails &details, QString &ret) const;
		static const QIcon& getFlagsIcon(FileStorageFlags flags) ;

		virtual QVariant displayRole(const DirDetails&,int) const = 0 ;
		virtual QVariant sortRole(const QModelIndex&,const DirDetails&,int) const =0;

		QVariant decorationRole(const DirDetails&,int) const ;
		QVariant filterRole(const DirDetails& details,int coln) const;

		uint32_t ageIndicator;

		QIcon categoryIcon;
		QIcon peerIcon;

		//class RemoteIndex
		//{
		//	public:
		//		RemoteIndex() {}
		//		RemoteIndex(std::string in_person,
		//				std::string in_path,
		//				int in_idx,
		//				int in_row,
		//				int in_column,
		//				std::string in_name,
		//				int in_size,
		//				int in_type,
		//				int in_ts, int in_rank)
		//			:id(in_person), path(in_path), parent(in_idx),
		//			row(in_row), column(in_column),
		//			name(in_name), size(in_size),
		//			type(in_type), timestamp(in_ts), rank(in_rank)
		//	{
		//		return;
		//	}
		//
		//		std::string id;
		//		std::string path;
		//		int parent;
		//		int row;
		//		int column;
		//
		//		/* display info */
		//		std::string name;
		//		int size;
		//		int type;
		//		int timestamp;
		//		int rank;
		//
		//};

		bool RemoteMode;

		//mutable int nIndex;
		//mutable std::vector<RemoteIndex> indexSet;

        // This material attempts to keep last request in cache, with no search cost.

        mutable DirDetails mDirDetails ;
        mutable bool mLastRemote ;
        mutable time_t mLastReq;

        bool mUpdating ;

		std::set<void*> mFilteredPointers ;
};

// This class shows the classical hierarchical directory view of shared files
// Columns are:
// 	file name     |    Size      |   Age
//
class TreeStyle_RDM: public RetroshareDirModel
{
	Q_OBJECT

	public:
		TreeStyle_RDM(bool mode);
		virtual ~TreeStyle_RDM() ;

	protected:

		//Overloaded from RetroshareDirModel
		virtual void update() ;
		virtual void updateRef(const QModelIndex&) const ;
		virtual QMenu* getContextMenu(QMenu* contextMenu) ;
		virtual QVariant displayRole(const DirDetails&,int) const ;
		virtual QVariant sortRole(const QModelIndex&,const DirDetails&,int) const ;

		//Overloaded from QAbstractItemModel
		virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex() ) const;
		virtual QModelIndex parent ( const QModelIndex & index ) const;

		virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
		virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
		virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	private slots:
		void showEmpty(const bool value);

	private:
		QAction *_showEmptyAct;
		bool _showEmpty;
	protected:
		mutable std::vector<int> _parentRow ; // used to store the real parent row for non empty child
};

// This class shows a flat list of all shared files
// Columns are:
// 	file name     |    Owner    |    Size      |   Age
//
class FlatStyle_RDM: public RetroshareDirModel
{
	Q_OBJECT 

	public:
		FlatStyle_RDM(bool mode);
		virtual ~FlatStyle_RDM() ;

		//Overloaded from RetroshareDirModel
		virtual void update() ;

		bool isMaxRefsTableSize(size_t* maxSize = NULL);

	protected slots:
		void updateRefs() ;

	protected:
 		//Overloaded from RetroshareDirModel
		virtual void postMods();/* Callback from Core */
		virtual void updateRef(const QModelIndex&) const {}
		virtual QVariant displayRole(const DirDetails&,int) const ;
		virtual QVariant sortRole(const QModelIndex&,const DirDetails&,int) const ;

		//Overloaded from QAbstractItemModel
		virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex() ) const;
		virtual QModelIndex parent ( const QModelIndex & index ) const;

		virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
		virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
		virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;


		QString computeDirectoryPath(const DirDetails& details) const ;

		mutable RsMutex _ref_mutex ;
		std::vector<void *> _ref_entries ; // used to store the refs to display
		std::vector<void *> _ref_stack ;   // used to store the refs to update
		bool _needs_update ;
		time_t _last_update ;
};


#endif
