#ifndef REMOTE_DIR_MODEL
#define REMOTE_DIR_MODEL

#include <QAbstractItemModel>
#include <vector>
#include <list>
#include <string>

class RemoteDirModel : public QAbstractItemModel
 {
     Q_OBJECT

 public:
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

#if 0  /****** REMOVED ******/
     void recommendSelected(QModelIndexList list);
     void recommendSelectedOnly(QModelIndexList list);
#endif 

     void openSelected(QModelIndexList list);

     void getFilePaths(QModelIndexList list, std::list<std::string> &fullpaths);

  public slots:

     void collapsed ( const QModelIndex & index ) { update(index); } 
     void expanded ( const QModelIndex & index ) { update(index); }

  /* Drag and Drop Functionality */
  public:

virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const;
virtual QStringList mimeTypes () const;

 private:
     void update (const QModelIndex &index );

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
