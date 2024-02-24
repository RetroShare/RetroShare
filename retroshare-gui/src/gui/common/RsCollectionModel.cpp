#include <string>

#include "RsCollectionModel.h"

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
        return mCollection.fileTree().directoryData(i.index).subdirs.size();
}

int RsCollectionModel::columnCount(const QModelIndex&) const
{
    return 5;
}

QVariant RsCollectionModel::headerData(int section, Qt::Orientation,int) const
{
    switch(section)
    {
    case 0: return tr("File");
    case 1: return tr("Path");
    case 2: return tr("Size");
    case 3: return tr("Hash");
    case 4: return tr("Count");
    default:
        return QVariant();
    }
}

