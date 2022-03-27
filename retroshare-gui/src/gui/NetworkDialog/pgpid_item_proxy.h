/*******************************************************************************
 * retroshare-gui/src/gui/NetworkDialog/pgpid_item_proxy.h                     *
 *                                                                             *
 * Copyright (C) 2018 by Retroshare Team     <retroshare.project@gmail.com>    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef PGPID_ITEM_PROXY_H
#define PGPID_ITEM_PROXY_H

#include "util/cxx11retrocompat.h"
#include "pgpid_item_model.h"

#include <QSortFilterProxyModel>

class pgpid_item_proxy :
        public QSortFilterProxyModel
{
    Q_OBJECT

public:
    pgpid_item_proxy(QObject *parent = nullptr);
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

public slots:
    void use_only_trusted_keys(bool val);

private:
    bool only_trusted_keys = false;
};

#endif // PGPID_ITEM_PROXY_H
