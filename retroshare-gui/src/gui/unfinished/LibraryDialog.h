/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008, defnax
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _SHAREDFILESDIALOG_H
#define _SHAREDFILESDIALOG_H

#include <QFileDialog>

#include "gui/mainpage.h"
#include "ui_LibraryDialog.h"

#include <retroshare/rstypes.h>
#include "gui/RemoteDirModel.h"

class LibraryDialog : public MainPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  LibraryDialog(QWidget *parent = 0);
  /** Default Destructor */


private slots:

  	void PopulateList();

  	void CallShareFilesBtn_library();
    void CallTileViewBtn_library();
    void CallShowDetailsBtn_library();
    void CallCreateAlbumBtn_library();
    void CallDeleteAlbumBtn_library();
    void CallFindBtn_library();

    void browseFile();
    void player();
    void PlayFrmList();
    void copyFile();
    void DeleteFile();
    void RenameFile();
    void StopRename();


signals:


private:


  /** Qt Designer generated object */
  Ui::LibraryDialog ui;


};

#endif

