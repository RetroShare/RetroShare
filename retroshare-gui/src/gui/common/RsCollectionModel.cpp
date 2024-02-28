#include <string>

#include "RsCollectionModel.h"

// #define DEBUG_COLLECTION_MODEL 1

static const int COLLECTION_MODEL_NB_COLUMN = 4;

RsCollectionModel::RsCollectionModel(const RsCollection& col, QObject *parent)
    : QAbstractItemModel(parent),mCollection(col)
{
    postMods();
}

#ifdef DEBUG_COLLECTION_MODEL
static std::ostream& operator<<(std::ostream& o,const RsCollectionModel::EntryIndex& i)
{
    return o << ((i.is_file)?("File"):"Dir") << " with index " << (int)i.index ;
}
static std::ostream& operator<<(std::ostream& o,const QModelIndex& i)
{
    return o << "QModelIndex (row " << i.row() << ", of ref " << i.internalId() << ")" ;
}
#endif

// Indernal Id is always a quintptr_t (basically a uint with the size of a pointer). Depending on the
// Indernal Id is always a quintptr_t (basically a uint with the size of a pointer). Depending on the
// architecture, the pointer may have 4 or 8 bytes. We use the low-level bit for type (0=dir, 1=file) and
// the remaining bits for the index (which will be accordingly understood as a FileIndex or a DirIndex)
// This way, index 0 is always the top dir.

bool RsCollectionModel::convertIndexToInternalId(const EntryIndex& e,quintptr& ref)
{
    ref = (e.index << 1) | e.is_file;
    return true;
}

bool RsCollectionModel::convertInternalIdToIndex(quintptr ref, EntryIndex& e)
{
    e.is_file = (bool)(ref & 1);
    e.index   = ref >> 1;
    return true;
}

int RsCollectionModel::rowCount(const QModelIndex& parent) const
{
#ifdef DEBUG_COLLECTION_MODEL
    std::cerr << "Asking rowCount of " << parent << std::endl;
#endif

    if(parent.column() >= COLLECTION_MODEL_NB_COLUMN)
        return 0;

    if(!parent.isValid())
    {
#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "  root! returning " << mCollection.fileTree().directoryData(0).subdirs.size()
                + mCollection.fileTree().directoryData(0).subfiles.size() << std::endl;
#endif

        return    mCollection.fileTree().directoryData(0).subdirs.size()
                + mCollection.fileTree().directoryData(0).subfiles.size();
    }

    EntryIndex i;
    if(!convertInternalIdToIndex(parent.internalId(),i))
        return 0;

    if(i.is_file)
    {
#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "  file: returning 0" << std::endl;
#endif
        return 0;
    }
    else
    {
#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "  dir: returning " << mCollection.fileTree().directoryData(i.index).subdirs.size() + mCollection.fileTree().directoryData(i.index).subfiles.size()
                  << std::endl;
#endif
        return mCollection.fileTree().directoryData(i.index).subdirs.size() + mCollection.fileTree().directoryData(i.index).subfiles.size();
    }
}

bool RsCollectionModel::hasChildren(const QModelIndex & parent) const
{
    if(!parent.isValid())
        return true;

    EntryIndex i;
    if(!convertInternalIdToIndex(parent.internalId(),i))
        return false;

    if(i.is_file)
        return false;
    else if(mCollection.fileTree().directoryData(i.index).subdirs.size() + mCollection.fileTree().directoryData(i.index).subfiles.size() > 0)
        return true;
    else
        return false;
}

int RsCollectionModel::columnCount(const QModelIndex&) const
{
    return COLLECTION_MODEL_NB_COLUMN;
}

QVariant RsCollectionModel::headerData(int section, Qt::Orientation,int role) const
{
    if(role == Qt::DisplayRole)
        switch(section)
        {
        case 0: return tr("File");
        case 1: return tr("Size");
        case 2: return tr("Hash");
        case 3: return tr("Count");
        default:
            return QVariant();
        }
    return QVariant();
}

QModelIndex RsCollectionModel::index(int row, int column, const QModelIndex & parent) const
{
    if(row < 0 || column < 0 || column >= columnCount(parent) || row >= rowCount(parent))
        return QModelIndex();

    EntryIndex i;

    if(!parent.isValid())	// root
    {
        i.is_file = false;
        i.index = 0;
    }
    else if(!convertInternalIdToIndex(parent.internalId(),i))
        return QModelIndex();

    if(i.is_file || i.index >= mCollection.fileTree().numDirs())
        return QModelIndex();

    const auto& parentData(mCollection.fileTree().directoryData(i.index));

    if((size_t)row < parentData.subdirs.size())
    {
        EntryIndex e;
        e.is_file = false;
        e.index = parentData.subdirs[row];

        quintptr ref;
        convertIndexToInternalId(e,ref);

#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "creating index for row " << row << " of parent " << parent << ". result is " << createIndex(row,column,ref) << std::endl;
#endif
        return createIndex(row,column,ref);
    }

    if((size_t)row < parentData.subdirs.size() + parentData.subfiles.size())
    {
        EntryIndex e;
        e.is_file = true;
        e.index = parentData.subfiles[row - parentData.subdirs.size()];

        quintptr ref;
        convertIndexToInternalId(e,ref);
#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "creating index for row " << row << " of parent " << parent << ". result is " << createIndex(row,column,ref) << std::endl;
#endif
        return createIndex(row,column,ref);
    }

    return QModelIndex();
}

QModelIndex RsCollectionModel::parent(const QModelIndex & index) const
{
    if(!index.isValid())
        return QModelIndex();

    EntryIndex i;
    if(!convertInternalIdToIndex(index.internalId(),i) || i.index==0)
        return QModelIndex();

    EntryIndex p;
    p.is_file = false;	// all parents are directories
    int row;

    if(i.is_file)
    {
        const auto it = mFileParents.find(i.index);
        if(it == mFileParents.end())
        {
            RsErr() << "Error: parent not found for index " << index.row() << ", " << index.column();
            return QModelIndex();
        }
        p.index = it->second.parent_index;
        row = it->second.parent_row;
    }
    else
    {
        const auto it = mDirParents.find(i.index);
        if(it == mDirParents.end())
        {
            RsErr() << "Error: parent not found for index " << index.row() << ", " << index.column();
            return QModelIndex();
        }
        p.index = it->second.parent_index;
        row = it->second.parent_row;
    }

    quintptr ref;
    convertIndexToInternalId(p,ref);

    return createIndex(row,0,ref);
}

QVariant RsCollectionModel::data(const QModelIndex& index, int role) const
{
    EntryIndex i;
    if(!convertInternalIdToIndex(index.internalId(),i))
        return QVariant();

#ifdef DEBUG_COLLECTION_MODEL
    std::cerr << "Asking data of " << i << std::endl;
#endif
    switch(role)
    {
    case Qt::DisplayRole: return displayRole(i,index.column());
    //case Qt::SortRole: return SortRole(i,index.column());
    case Qt::DecorationRole: return decorationRole(i,index.column());
    default:
        return QVariant();
    }
}

QVariant RsCollectionModel::displayRole(const EntryIndex& i,int col) const
{
    switch(col)
    {
    case 0: return (i.is_file)?
                    (QString::fromUtf8(mCollection.fileTree().fileData(i.index).name.c_str()))
                  : (QString::fromUtf8(mCollection.fileTree().directoryData(i.index).name.c_str()));

    case 1: if(i.is_file)
                return QVariant((qulonglong)mCollection.fileTree().fileData(i.index).size) ;

            {
                auto it = mDirSizes.find(i.index);

                if(it == mDirSizes.end())
                    return QVariant();
                else
                    return QVariant((qulonglong)it->second);
            }

    case 2: return (i.is_file)?
                    QString::fromStdString(mCollection.fileTree().fileData(i.index).hash.toStdString())
                   :QVariant();

    case 3: return (i.is_file)?((qulonglong)1):((qulonglong)(mCollection.fileTree().directoryData(i.index).subdirs.size()));
    }
    return QVariant();
}
QVariant RsCollectionModel::sortRole(const EntryIndex& i,int col) const
{
    return QVariant();
}
QVariant RsCollectionModel::decorationRole(const EntryIndex& i,int col) const
{
    return QVariant();
}

void RsCollectionModel::preMods()
{
    mUpdating = true;
    emit layoutAboutToBeChanged();
}
void RsCollectionModel::postMods()
{
    // update all the local structures

    mDirParents.clear();
    mFileParents.clear();
    mDirSizes.clear();

#ifdef DEBUG_COLLECTION_MODEL
    std::cerr << "Updating from tree: " << std::endl;
#endif
    uint64_t s;
    recursUpdateLocalStructures(mCollection.fileTree().root(),s,0);

    mUpdating = false;
    emit layoutChanged();
}

void RsCollectionModel::recursUpdateLocalStructures(RsFileTree::DirIndex dir_index,uint64_t& total_size,int depth)
{
    total_size = 0;

    const auto& dd(mCollection.fileTree().directoryData(dir_index));

    for(uint32_t i=0;i<dd.subdirs.size();++i)
    {
#ifdef DEBUG_COLLECTION_MODEL
        for(int j=0;j<depth;++j) std::cerr << "  ";
        std::cerr << "Dir \"" << mCollection.fileTree().directoryData(dd.subdirs[i]).name << "\"" << std::endl ;
#endif

        uint64_t ss;
        recursUpdateLocalStructures(dd.subdirs[i],ss,depth+1);
        total_size += ss;

        auto& ref(mDirParents[dd.subdirs[i]]);

        ref.parent_index = dir_index;
        ref.parent_row   = i;
    }

    for(uint32_t i=0;i<dd.subfiles.size();++i)
    {
#ifdef DEBUG_COLLECTION_MODEL
        for(int j=0;j<depth;++j) std::cerr << "  ";
        std::cerr << "File \"" << mCollection.fileTree().fileData(dd.subfiles[i]).name << "\"" << std::endl;
#endif

        total_size += mCollection.fileTree().fileData(dd.subfiles[i]).size;

        auto& ref(mFileParents[dd.subfiles[i]]);

        ref.parent_index = dir_index;
        ref.parent_row   = i + dd.subdirs.size();
    }

    mDirSizes[dir_index] = total_size;
}






