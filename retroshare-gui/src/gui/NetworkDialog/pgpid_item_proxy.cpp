#include "pgpid_item_proxy.h"

pgpid_item_proxy::pgpid_item_proxy(QObject *parent) :
    //QAbstractProxyModel(parent)
    QSortFilterProxyModel(parent)
{

}


/*QModelIndex pgpid_item_proxy::mapFromSource(const QModelIndex &sourceIndex) const
{
    if(sourceIndex.isValid())
        return createIndex(sourceIndex.row(), sourceIndex.column(), sourceIndex.internalPointer());
    else
        return QModelIndex();
}


QModelIndex pgpid_item_proxy::mapToSource(const QModelIndex &proxyIndex) const
{
    if(proxyIndex.isValid())
        return sourceModel()->index(proxyIndex.row(), proxyIndex.column());
    else
        return QModelIndex();}


QModelIndex pgpid_item_proxy::index(int row, int column, const QModelIndex &parent) const
{
    const QModelIndex sourceParent = mapToSource(parent);
    const QModelIndex sourceIndex = sourceModel()->index(row, column, sourceParent);
    return mapFromSource(sourceIndex);
}

QModelIndex pgpid_item_proxy::parent(const QModelIndex &child) const
{
    const QModelIndex sourceIndex = mapToSource(child);
    const QModelIndex sourceParent = sourceIndex.parent();
    return mapFromSource(sourceParent);
}
int pgpid_item_proxy::rowCount(const QModelIndex &parent) const
{
    //TODO:
    return sourceModel()->rowCount(parent);
}
int pgpid_item_proxy::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(parent);
}
*/
