#include <string>

#include "RsCollectionModel.h"

// #define DEBUG_COLLECTION_MODEL 1

static const int COLLECTION_MODEL_FILENAME  = 0;
static const int COLLECTION_MODEL_SIZE      = 1;
static const int COLLECTION_MODEL_HASH      = 2;
static const int COLLECTION_MODEL_COUNT     = 3;
static const int COLLECTION_MODEL_NB_COLUMN = 4;

RsCollectionModel::RsCollectionModel(const RsCollection& col, QObject *parent)
    : QAbstractItemModel(parent),mCollection(col)
{
    postMods();
}

static std::ostream& operator<<(std::ostream& o,const RsCollectionModel::EntryIndex& i)
{
    return o << ((i.is_file)?("File"):"Dir") << " with index " << (int)i.index ;
}
static std::ostream& operator<<(std::ostream& o,const QModelIndex& i)
{
    return o << "QModelIndex (row " << i.row() << ", of ref " << i.internalId() << ")" ;
}
#ifdef DEBUG_COLLECTION_MODEL
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

Qt::ItemFlags RsCollectionModel::flags ( const QModelIndex & index ) const
{
    if(index.isValid() && index.column() == COLLECTION_MODEL_FILENAME)
    {
        EntryIndex e;

        if(!convertInternalIdToIndex(index.internalId(),e))
            return QAbstractItemModel::flags(index) ;

        if(e.is_file)
            return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
        else
            return QAbstractItemModel::flags(index) | Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable;
    }

    return QAbstractItemModel::flags(index) ;
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
        p.index = mFileInfos[i.index].parent_index;
        row = mFileInfos[i.index].parent_row;
    }
    else
    {
        p.index = mDirInfos[i.index].parent_index;
        row = mDirInfos[i.index].parent_row;
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
    case Qt::DisplayRole:    return displayRole(i,index.column());
    case Qt::DecorationRole: return decorationRole(i,index.column());
    case Qt::CheckStateRole: return checkStateRole(i,index.column());
    default:
        return QVariant();
    }
}

bool RsCollectionModel::setData(const QModelIndex& index,const QVariant& value,int role)
{
    if(!index.isValid())
        return false;

    EntryIndex e;

    if(role==Qt::CheckStateRole && convertInternalIdToIndex(index.internalId(), e))
    {
//#ifdef DEBUG_COLLECTION_MODEL
        std::cerr << "Setting check state of item " << index << " to " << value.toBool() << std::endl;
//#endif
        RsFileTree::DirIndex dir_index ;

        if(e.is_file)
        {
            mFileInfos[e.index].is_checked = value.toBool();
            dir_index = mFileInfos[e.index].parent_index;
        }
        else
        {
            std::function<void(RsFileTree::DirIndex,bool)> recursSetCheckFlag = [&](RsFileTree::DirIndex index,bool s) -> void
            {
                mDirInfos[index].check_state = (s)?SELECTED:UNSELECTED;
                auto& dir_data(mCollection.fileTree().directoryData(index));

                mDirInfos[index].total_size = 0;
                mDirInfos[index].total_count = 0;

                for(uint32_t i=0;i<dir_data.subdirs.size();++i)
                {
                    recursSetCheckFlag(dir_data.subdirs[i],s);
                    mDirInfos[index].total_size += mDirInfos[dir_data.subdirs[i]].total_size ;
                    ++mDirInfos[index].total_count;
                }

                for(uint32_t i=0;i<dir_data.subfiles.size();++i)
                {
                    mFileInfos[dir_data.subfiles[i]].is_checked = s;

                    if(s)
                    {
                        mDirInfos[index].total_size += mCollection.fileTree().fileData(dir_data.subfiles[i]).size;
                        ++mDirInfos[index].total_count;
                    }
                }
            };
            recursSetCheckFlag(e.index,value.toBool());
            dir_index = mDirInfos[e.index].parent_index;
        }

        // now go up the directories and update the check tristate flag, depending on whether the children are all checked/unchecked or mixed.

        do
        {
            auto& dit(mDirInfos[dir_index]);

            const RsFileTree::DirData& dir_data(mCollection.fileTree().directoryData(dir_index));	// get the directory data

            bool locally_all_checked = true;
            bool locally_all_unchecked = true;

            dit.total_size = 0;
            dit.total_count = 0;

            for(uint32_t i=0;i<dir_data.subdirs.size();++i)
            {
                const auto& dit2(mDirInfos[dir_data.subdirs[i]]);
                dit.total_size += dit2.total_size;
                dit.total_count += dit2.total_count;

                if(dit2.check_state == UNSELECTED || dit2.check_state == PARTIALLY_SELECTED)
                    locally_all_checked   = false;

                if(dit2.check_state ==   SELECTED || dit2.check_state == PARTIALLY_SELECTED)
                    locally_all_unchecked = false;
            }
            for(uint32_t i=0;i<dir_data.subfiles.size();++i)
            {
                const auto& fit2(mFileInfos[dir_data.subfiles[i]]);

                if(fit2.is_checked)
                {
                    dit.total_size += mCollection.fileTree().fileData(dir_data.subfiles[i]).size;
                    ++dit.total_count;
                    locally_all_unchecked = false;
                }
                else
                    locally_all_checked = false;
            }

            if(locally_all_checked)
                dit.check_state = SELECTED;
            else if(locally_all_unchecked)
                dit.check_state = UNSELECTED;
            else
                dit.check_state = PARTIALLY_SELECTED;

            if(dir_index == mCollection.fileTree().root())
                break;
            else
                dir_index = dit.parent_index;	// get the directory data

        }
        while(true);

        const auto& top_dir(mCollection.fileTree().directoryData(mCollection.fileTree().root()));
        emit dataChanged(createIndex(0,0,(void*)NULL),
                         createIndex(top_dir.subdirs.size() + top_dir.subfiles.size() - 1,
                                     COLLECTION_MODEL_NB_COLUMN-1,
                                     (void*)NULL),
                         { Qt::CheckStateRole });

        emit sizesChanged();

        return true;
    }
    else
        return QAbstractItemModel::setData(index,value,role);
}

QVariant RsCollectionModel::checkStateRole(const EntryIndex& i,int col) const
{
    if(col == COLLECTION_MODEL_FILENAME)
    {
        if(i.is_file)
        {
            std::cerr<< "entry is file, checkstate = " << (int)mFileInfos[i.index].is_checked << std::endl;
            if(mFileInfos[i.index].is_checked)
                return QVariant(Qt::Checked);
            else
                return QVariant(Qt::Unchecked);
        }
        else
        {
            std::cerr<< "entry is dir, checkstate = " << (int)mDirInfos[i.index].check_state << std::endl;

            switch(mDirInfos[i.index].check_state)
            {
            case SELECTED: return QVariant::fromValue((int)Qt::Checked);
            case PARTIALLY_SELECTED: return QVariant::fromValue((int)Qt::PartiallyChecked);
            default:
            case UNSELECTED: return QVariant::fromValue((int)Qt::Unchecked);
            }
        }
    }
    else
        return QVariant();
}
QVariant RsCollectionModel::displayRole(const EntryIndex& i,int col) const
{
    switch(col)
    {
    case COLLECTION_MODEL_FILENAME: return (i.is_file)?
                    (QString::fromUtf8(mCollection.fileTree().fileData(i.index).name.c_str()))
                  : (QString::fromUtf8(mCollection.fileTree().directoryData(i.index).name.c_str()));

    case COLLECTION_MODEL_SIZE: if(i.is_file)
            return QVariant((qulonglong)mCollection.fileTree().fileData(i.index).size) ;
        else
            return QVariant((qulonglong)mDirInfos[i.index].total_size);

    case COLLECTION_MODEL_HASH: return (i.is_file)?
                    QString::fromStdString(mCollection.fileTree().fileData(i.index).hash.toStdString())
                  :QVariant();

    case COLLECTION_MODEL_COUNT: if(i.is_file)
            return (qulonglong)mFileInfos[i.index].is_checked;
        else
            return (qulonglong)(mDirInfos[i.index].total_count);
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

    mDirInfos.clear();
    mFileInfos.clear();

    mDirInfos.resize(mCollection.fileTree().numDirs());
    mFileInfos.resize(mCollection.fileTree().numFiles());

    mDirInfos[0].parent_index = 0;

#ifdef DEBUG_COLLECTION_MODEL
    std::cerr << "Updating from tree: " << std::endl;
#endif
    recursUpdateLocalStructures(mCollection.fileTree().root(),0);

    mUpdating = false;
    emit layoutChanged();
    emit sizesChanged();
}

void RsCollectionModel::recursUpdateLocalStructures(RsFileTree::DirIndex dir_index,int depth)
{
    uint64_t total_size = 0;
    uint64_t total_count = 0;
    bool all_checked = true;
    bool all_unchecked = false;

    const auto& dd(mCollection.fileTree().directoryData(dir_index));

    for(uint32_t i=0;i<dd.subfiles.size();++i)
    {
#ifdef DEBUG_COLLECTION_MODEL
        for(int j=0;j<depth;++j) std::cerr << "  ";
        std::cerr << "File \"" << mCollection.fileTree().fileData(dd.subfiles[i]).name << "\"" << std::endl;
#endif
        auto& ref(mFileInfos[dd.subfiles[i]]);

        if(ref.is_checked)
        {
            total_size += mCollection.fileTree().fileData(dd.subfiles[i]).size;
            ++total_count;
        }

        ref.parent_index = dir_index;
        ref.parent_row   = i + dd.subdirs.size();

        all_checked = all_checked && ref.is_checked;
        all_unchecked = all_unchecked && !ref.is_checked;
    }

    for(uint32_t i=0;i<dd.subdirs.size();++i)
    {
#ifdef DEBUG_COLLECTION_MODEL
        for(int j=0;j<depth;++j) std::cerr << "  ";
        std::cerr << "Dir \"" << mCollection.fileTree().directoryData(dd.subdirs[i]).name << "\"" << std::endl ;
#endif

        recursUpdateLocalStructures(dd.subdirs[i],depth+1);

        auto& ref(mDirInfos[dd.subdirs[i]]);

        total_size  += ref.total_size;
        total_count += ref.total_count;

        ref.parent_index = dir_index;
        ref.parent_row   = i;

        all_checked   = all_checked && (ref.check_state == SELECTED);
        all_unchecked = all_unchecked && (ref.check_state == UNSELECTED);
    }

    auto& r(mDirInfos[dir_index]);

    r.total_size  = total_size;
    r.total_count = total_count;

    if(all_checked)
        r.check_state = SELECTED;
    else if(all_unchecked)
        r.check_state = UNSELECTED;
    else
        r.check_state = PARTIALLY_SELECTED;
}






