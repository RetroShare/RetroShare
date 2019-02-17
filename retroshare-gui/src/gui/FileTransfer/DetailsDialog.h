/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/DetailsDialog.h                         *
 *                                                                             *
 * Copyright 2010 by Retroshare Team <retroshare.project@gmail.com>            *
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

#ifndef _DETAILSDIALOG_H
#define _DETAILSDIALOG_H

#include <stdint.h>
#include "ui_DetailsDialog.h"
#include <retroshare/rstypes.h>

struct FileChunksInfo ;

class DetailsDialog : public QDialog
{
  Q_OBJECT

public:
  /** Default constructor */
  DetailsDialog(QWidget *parent = 0);
  /** Default destructor */
  ~DetailsDialog() {}
  
    void setFileHash(const RsFileHash &hash) ;

public slots:
  /** Overloaded QWidget.show */
  void show();
	void copyLink() ;
  
private slots:
  void on_ok_dButton_clicked();
  void on_cancel_dButton_clicked();
  
private:

	class QStandardItemModel *CommentsModel;

     RsFileHash _file_hash ;

  
  /** Qt Designer generated object */
  Ui::DetailsDialog ui;
};

#endif

