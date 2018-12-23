/*******************************************************************************
 * retroshare-gui/src/gui/PhotoShare/AlbumDialog.h                             *
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

#ifndef ALBUMDIALOG_H
#define ALBUMDIALOG_H

#include <QDialog>
#include "retroshare/rsphoto.h"
#include "util/TokenQueue.h"
#include "PhotoShareItemHolder.h"
#include "PhotoItem.h"
#include "PhotoDrop.h"

namespace Ui {
    class AlbumDialog;
}

class AlbumDialog : public QDialog, public PhotoShareItemHolder
{
    Q_OBJECT

public:
    explicit AlbumDialog(const RsPhotoAlbum& album, TokenQueue* photoQueue, RsPhoto* rs_Photo, QWidget *parent = 0);
    ~AlbumDialog();

    void notifySelection(PhotoShareItem* selection);

private:

    void setUp();

private slots:

    void updateAlbumPhotos();
    void deletePhoto();
    void editPhoto();
private:
    Ui::AlbumDialog *ui;
    RsPhoto* mRsPhoto;
    TokenQueue* mPhotoQueue;
    RsPhotoAlbum mAlbum;
    PhotoDrop* mPhotoDrop;
    PhotoItem* mPhotoSelected;
};

#endif // ALBUMDIALOG_H
