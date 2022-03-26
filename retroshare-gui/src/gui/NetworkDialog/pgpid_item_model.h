/*******************************************************************************
 * retroshare-gui/src/gui/NetworkDialog/pgpid_item_model.h                     *
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

#ifndef KEY_ITEM_MODEL_H
#define KEY_ITEM_MODEL_H

#include <QAbstractItemModel>
#include <retroshare/rspeers.h>
#include <QColor>

class pgpid_item_model : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit pgpid_item_model(std::list<RsPgpId> &neighs, float &font_height, QObject *parent = nullptr);

    // Header:
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const ;


    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const ;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const ;

    void setBackgroundColorSelf(QColor color) { mBackgroundColorSelf = color; }
    void setBackgroundColorOwnSign(QColor color) { mBackgroundColorOwnSign = color; }
    void setBackgroundColorAcceptConnection(QColor color) { mBackgroundColorAcceptConnection = color; }
    void setBackgroundColorHasSignedMe(QColor color) { mBackgroundColorHasSignedMe = color; }
    void setBackgroundColorDenied(QColor color) { mBackgroundColorDenied = color; }
    void setTextColor(QColor color) { mTextColor = color; }


public slots:
    void data_updated(std::list<RsPgpId> &new_neighs);

private:
    std::list<RsPgpId> &neighs;
    float font_height;
    QColor mBackgroundColorSelf;
    QColor mBackgroundColorOwnSign;
    QColor mBackgroundColorAcceptConnection;
    QColor mBackgroundColorHasSignedMe;
    QColor mBackgroundColorDenied;
    QColor mTextColor;
};

#endif // KEY_ITEM_MODEL_H
