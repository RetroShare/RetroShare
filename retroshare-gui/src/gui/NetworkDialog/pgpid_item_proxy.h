#ifndef PGPID_ITEM_PROXY_H
#define PGPID_ITEM_PROXY_H

#include <QSortFilterProxyModel>

class pgpid_item_proxy :
        //public  QAbstractProxyModel
        public QSortFilterProxyModel
{
    Q_OBJECT

public:
    pgpid_item_proxy(QObject *parent = nullptr);
/*    virtual QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
*/
};

#endif // PGPID_ITEM_PROXY_H
