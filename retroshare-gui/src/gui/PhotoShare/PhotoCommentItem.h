/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/PhotoCommentItem.h                        *
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

#ifndef PHOTOCOMMENTITEM_H
#define PHOTOCOMMENTITEM_H

#include <QWidget>
#include "retroshare/rsphoto.h"

namespace Ui {
    class PhotoCommentItem;
}

class PhotoCommentItem : public QWidget
{
    Q_OBJECT

public:
    explicit PhotoCommentItem(const RsPhotoComment& comment, QWidget *parent = 0);
    ~PhotoCommentItem();

    const RsPhotoComment& getComment();

private:

    void setUp();
private:
    Ui::PhotoCommentItem *ui;
    RsPhotoComment mComment;
};

#endif // PHOTOCOMMENTITEM_H
