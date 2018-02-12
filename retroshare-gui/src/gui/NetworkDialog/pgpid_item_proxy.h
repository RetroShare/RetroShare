#ifndef PGPID_ITEM_PROXY_H
#define PGPID_ITEM_PROXY_H

#include "util/cxx11retrocompat.h"

#include <QSortFilterProxyModel>

class pgpid_item_proxy :
        public QSortFilterProxyModel
{
    Q_OBJECT

public:
    pgpid_item_proxy(QObject *parent = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
public slots:
    void use_only_trusted_keys(bool val);

private:
    bool only_trusted_keys = false;
};

#endif // PGPID_ITEM_PROXY_H
