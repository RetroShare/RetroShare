#include <QAbstractItemModel>

#include "RsCollection.h"

class RsCollectionModel: public QAbstractItemModel
{
    Q_OBJECT

    public:
        enum Roles{ FileNameRole = Qt::UserRole+1, SortRole = Qt::UserRole+2, FilterRole = Qt::UserRole+3 };

        RsCollectionModel(bool mode, QObject *parent = 0);
        virtual ~RsCollectionModel() ;


        /* Callback from Core */
        void preMods();
        void postMods();

        /* Callback from GUI */

        void update() {}
        void filterItems(const std::list<std::string>& keywords, uint32_t& found) ;

        // Overloaded from QAbstractItemModel
        virtual Qt::ItemFlags flags ( const QModelIndex & index ) const override;
        virtual QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex() ) const override;
        virtual QModelIndex parent ( const QModelIndex & index ) const override;

        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        virtual bool hasChildren(const QModelIndex & parent = QModelIndex()) const override;

        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        virtual QStringList mimeTypes () const override;
        virtual QMimeData * mimeData ( const QModelIndexList & indexes ) const override;
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
        virtual Qt::DropActions supportedDragActions() const override;
#endif

    protected:
        struct EntryIndex {
            bool is_file;		// false=dir, true=file
            uint64_t index;
        };
        static bool convertIndexToInternalId(const EntryIndex& e,quintptr& ref);
        static bool convertInternalIdToIndex(quintptr ref, EntryIndex& e);

        // virtual QVariant displayRole(const DirDetails&,int) const = 0 ;
        // virtual QVariant sortRole(const QModelIndex&,const DirDetails&,int) const =0;

        // QVariant decorationRole(const DirDetails&,int) const ;
        // QVariant filterRole(const DirDetails& details,int coln) const;

        bool mUpdating ;

        const RsCollection& mCollection;

        // std::set<void*> mFilteredPointers ;
};
