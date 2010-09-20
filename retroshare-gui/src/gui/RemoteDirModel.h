/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef REMOTE_DIR_MODEL
#define REMOTE_DIR_MODEL

#include <QAbstractItemModel>
#include <QIcon>
#include <vector>
#include <stdint.h>

class DirDetails;

static const uint32_t IND_LAST_DAY    = 3600*24 ;
static const uint32_t IND_LAST_WEEK   = 3600*24*7 ;
static const uint32_t IND_LAST_MONTH  = 3600*24*31	; // I know, this is approximate
static const uint32_t IND_ALWAYS      = ~(uint32_t)0 ;

class RemoteDirModel : public QAbstractItemModel
{
	Q_OBJECT

	public:
		enum Roles{ FileNameRole = Qt::UserRole+1, SortRole = Qt::UserRole+2 };

		RemoteDirModel(bool mode, QObject *parent = 0);

		/* These are all overloaded Virtual Functions */
		int rowCount(const QModelIndex &parent = QModelIndex()) const;
		int columnCount(const QModelIndex &parent = QModelIndex()) const;

		QVariant data(const QModelIndex &index, int role) const;
		QVariant headerData(int section, Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;

		QModelIndex index(int row, int column,
				const QModelIndex & parent = QModelIndex() ) const;
		QModelIndex parent ( const QModelIndex & index ) const;

		Qt::ItemFlags flags ( const QModelIndex & index ) const;
		bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

		/* Callback from Core */
		void preMods();
		void postMods();

		/* Callback from GUI */
		void downloadSelected(QModelIndexList list);

		void getDirDetailsFromSelect (QModelIndexList list, std::vector <DirDetails>& dirVec);

		bool isDir ( const QModelIndex & index ) const ;
		//void openFile(QModelIndex fileIndex, const QString command);

		//#if 0  /****** REMOVED ******/
		//     void recommendSelected(QModelIndexList list);
		//     void recommendSelectedOnly(QModelIndexList list);
		//#endif
		void getFileInfoFromIndexList(const QModelIndexList& list, std::list<DirDetails>& files_info) ;

		void openSelected(QModelIndexList list, bool openFolder);

		void getFilePaths(QModelIndexList list, std::list<std::string> &fullpaths);

		void changeAgeIndicator(uint32_t indicator) { ageIndicator = indicator; }


		public slots:

//			void collapsed ( const QModelIndex & index ) { update(index); }
//		void expanded ( const QModelIndex & index ) { update(index); }

		/* Drag and Drop Functionality */
	public:

		virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
		virtual QStringList mimeTypes () const;

	private:
//		void update (const QModelIndex &index );
		void treeStyle();
		void downloadDirectory(const DirDetails & details, int prefixLen);
		static QString getFlagsString(uint32_t) ;
		QString getAgeIndicatorString(const DirDetails &) const;
		void getAgeIndicatorRec(DirDetails &details, QString &ret) const;

		uint32_t ageIndicator;

		QIcon categoryIcon;
		QIcon peerIcon;

		class RemoteIndex
		{
			public:
				RemoteIndex() {}
				RemoteIndex(std::string in_person,
						std::string in_path,
						int in_idx,
						int in_row,
						int in_column,
						std::string in_name,
						int in_size,
						int in_type,
						int in_ts, int in_rank)
					:id(in_person), path(in_path), parent(in_idx),
					row(in_row), column(in_column),
					name(in_name), size(in_size),
					type(in_type), timestamp(in_ts), rank(in_rank)
			{
				return;
			}

				std::string id;
				std::string path;
				int parent;
				int row;
				int column;

				/* display info */
				std::string name;
				int size;
				int type;
				int timestamp;
				int rank;

		};

		bool RemoteMode;

		mutable int nIndex;
		mutable std::vector<RemoteIndex> indexSet;

};

#endif
