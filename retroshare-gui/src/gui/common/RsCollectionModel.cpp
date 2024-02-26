#include <string>

#include "RsCollectionModel.h"

RsCollectionModel::RsCollectionModel(const RsCollection& col, QObject *parent)
    : QAbstractItemModel(parent),mCollection(col)
{}

// Indernal Id is always a quintptr_t (basically a uint with the size of a pointer). Depending on the
// architecture, the pointer may have 4 or 8 bytes. We use the low-level bit for type (0=dir, 1=file) and
// the remaining bits for the index (which will be accordingly understood as a FileIndex or a DirIndex)
// This way, index 0 is always the top dir.

bool RsCollectionModel::convertIndexToInternalId(const EntryIndex& e,quintptr& ref)
{
    ref = (e.index << 1) || e.is_file;
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
    if(!parent.isValid())
        return mCollection.fileTree().root();

    EntryIndex i;
    if(!convertInternalIdToIndex(parent.internalId(),i))
        return 0;

    if(i.is_file)
        return 0;
    else
        return mCollection.fileTree().directoryData(i.index).subdirs.size() + mCollection.fileTree().directoryData(i.index).subfiles.size();
}

bool RsCollectionModel::hasChildren(const QModelIndex & parent) const
{
    if(!parent.isValid())
        return mCollection.fileTree().root();

    EntryIndex i;
    if(!convertInternalIdToIndex(parent.internalId(),i))
        return 0;

    if(i.is_file)
        return false;
    else
        return mCollection.fileTree().directoryData(i.index).subdirs.size() + mCollection.fileTree().directoryData(i.index).subfiles.size() > 0;
}

int RsCollectionModel::columnCount(const QModelIndex&) const
{
    return 4;
}

QVariant RsCollectionModel::headerData(int section, Qt::Orientation,int) const
{
    switch(section)
    {
    case 0: return tr("File");
    case 1: return tr("Size");
    case 2: return tr("Hash");
    case 3: return tr("Count");
    default:
        return QVariant();
    }
}

QModelIndex RsCollectionModel::index(int row, int column, const QModelIndex & parent) const
{
    EntryIndex i;
    if(!convertInternalIdToIndex(parent.internalId(),i))
        return QModelIndex();

    if(i.is_file || i.index >= mCollection.fileTree().numDirs())
        return QModelIndex();

    const auto& parentData(mCollection.fileTree().directoryData(i.index));

    if(row < 0)
        return QModelIndex();

    if((size_t)row < parentData.subdirs.size())
    {
        EntryIndex e;
        e.is_file = false;
        e.index = parentData.subdirs[row];

        quintptr ref;
        convertIndexToInternalId(e,ref);
        return createIndex(row,column,ref);
    }

    if((size_t)row < parentData.subdirs.size() + parentData.subfiles.size())
    {
        EntryIndex e;
        e.is_file = true;
        e.index = parentData.subfiles[row - parentData.subdirs.size()];

        quintptr ref;
        convertIndexToInternalId(e,ref);
        return createIndex(row,column,ref);
    }

    return QModelIndex();
}

QModelIndex RsCollectionModel::parent(const QModelIndex & index) const
{
    EntryIndex i;
    if(!convertInternalIdToIndex(index.internalId(),i))
        return QModelIndex();

    EntryIndex p;
    p.is_file = false;	// all parents are directories

    if(i.is_file)
    {
        const auto it = mFileParents.find(i.index);
        if(it == mFileParents.end())
        {
            RsErr() << "Error: parent not found for index " << index.row() << ", " << index.column();
            return QModelIndex();
        }
        p.index = it->second;
    }
    else
    {
        const auto it = mDirParents.find(i.index);
        if(it == mDirParents.end())
        {
            RsErr() << "Error: parent not found for index " << index.row() << ", " << index.column();
            return QModelIndex();
        }
        p.index = it->second;
    }

    quintptr ref;
    convertIndexToInternalId(p,ref);
    return createIndex(0,index.column(),ref);
}

QVariant RsCollectionModel::data(const QModelIndex& index, int role) const
{
    EntryIndex i;
    if(!convertInternalIdToIndex(index.internalId(),i))
        return QVariant();

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

    uint64_t s;
    recursUpdateLocalStructures(mCollection.fileTree().root(),s);

    mUpdating = false;
    emit layoutChanged();
}

void RsCollectionModel::recursUpdateLocalStructures(RsFileTree::DirIndex dir_index,uint64_t& total_size)
{
    total_size = 0;

    const auto& dd(mCollection.fileTree().directoryData(dir_index));

    for(uint32_t i=0;i<dd.subfiles.size();++i)
    {
        total_size += mCollection.fileTree().fileData(dd.subfiles[i]).size;
        mFileParents[dd.subfiles[i]] = dir_index;
    }

    for(uint32_t i=0;i<dd.subdirs.size();++i)
    {
        uint64_t ss;
        recursUpdateLocalStructures(dd.subdirs[i],ss);
        total_size += ss;
        mDirParents[dd.subdirs[i]] = dir_index;
    }
    mDirSizes[dir_index] = total_size;
}






