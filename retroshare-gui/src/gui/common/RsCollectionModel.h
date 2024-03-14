#include <QAbstractItemModel>

#include "RsCollection.h"

class RsCollectionModel: public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum Roles{ FileNameRole = Qt::UserRole+1, SortRole = Qt::UserRole+2, FilterRole = Qt::UserRole+3 };

        RsCollectionModel(const RsCollection& col, QObject *parent = 0);
        virtual ~RsCollectionModel() = default;

        /* Callback from Core */
        void preMods();			// always call this before updating the RsCollection!
        void postMods();		// always call this after updating the RsCollection!

        /* Callback from GUI */

        void update() ;
        void filterItems(const std::list<std::string>& keywords, uint32_t& found) ;

        // Overloaded from QAbstractItemModel
        virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
        virtual QModelIndex parent ( const QModelIndex & index ) const override;

        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const override;

        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual bool setData(const QModelIndex& index,const QVariant& value,int role) override;
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const override;
#ifdef TODO
        virtual QStringList mimeTypes () const override;
        virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const override;
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
        virtual Qt::DropActions supportedDragActions() const override;
#endif
#endif

        struct EntryIndex {
            bool is_file;		// false=dir, true=file
            uint64_t index;
        };
        uint64_t totalSize() const { return mDirInfos[0].total_size; }
        uint64_t totalSelected() const { return mDirInfos[0].total_count; }

        void notifyFilesBeingHashed(const std::list<RsFileHash>& files);
        void fileHashingFinished(const RsFileHash& hash);
        bool isChecked(EntryIndex);
    signals:
        void sizesChanged();	// tells that the total size of the top level dir has changed (due to selection)

    private:
        static bool convertIndexToInternalId(const EntryIndex& e,quintptr& ref);
        static bool convertInternalIdToIndex(quintptr ref, EntryIndex& e);

        void recursUpdateLocalStructures(RsFileTree::DirIndex dir_index, int depth);

        QVariant displayRole(const EntryIndex&,int col) const ;
        QVariant sortRole(const EntryIndex&,int col) const ;
        QVariant decorationRole(const EntryIndex&,int col) const ;
        QVariant checkStateRole(const EntryIndex& i,int col) const;
        QVariant textColorRole(const EntryIndex& i,int col) const;
        //QVariant filterRole(const DirDetails& details,int coln) const;

        void debugDump();

        bool mUpdating ;

        const RsCollection& mCollection;

        enum DirCheckState: uint8_t {
            UNSELECTED            = 0x00,
            PARTIALLY_SELECTED    = 0x01,
            SELECTED              = 0x02,
        };

        struct ModelDirInfo {
             ModelDirInfo() :parent_index(0),parent_row(0),check_state(SELECTED),total_size(0),total_count(0){}

             RsFileTree::DirIndex parent_index;	// index of the parent
             RsFileTree::DirIndex parent_row; // row of that child, in this parent
             DirCheckState check_state;
             uint64_t total_size;
             uint64_t total_count;
        };

        struct ModelFileInfo {
                ModelFileInfo() :parent_index(0),parent_row(0),is_checked(true){}

             RsFileTree::DirIndex parent_index;	// index of the parent
             RsFileTree::DirIndex parent_row; // row of that child, in this parent
            bool is_checked;
        };

        std::vector<ModelFileInfo> mFileInfos;
        std::vector<ModelDirInfo> mDirInfos;

        std::set<RsFileHash> mFilesBeingHashed;

        // std::set<void*> mFilteredPointers ;
};
