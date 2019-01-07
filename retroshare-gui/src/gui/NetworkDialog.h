/*******************************************************************************
 * gui/NetworkDialog.h                                                         *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#ifndef _CONNECTIONSDIALOG_H
#define _CONNECTIONSDIALOG_H

#include "ui_NetworkDialog.h"
#include "RsAutoUpdatePage.h"
#include "gui/NetworkDialog/pgpid_item_model.h"
#include "gui/NetworkDialog/pgpid_item_proxy.h"

//tmp
class  QTreeWidgetItem;

class RSTreeWidgetItemCompareRole ;


class NetworkDialog : public RsAutoUpdatePage
{
  Q_OBJECT

  Q_PROPERTY(QColor backgroundColorSelf READ backgroundColorSelf WRITE setBackgroundColorSelf)
  Q_PROPERTY(QColor backgroundColorOwnSign READ backgroundColorOwnSign WRITE setBackgroundColorOwnSign)
  Q_PROPERTY(QColor backgroundColorAcceptConnection READ backgroundColorAcceptConnection WRITE setBackgroundColorAcceptConnection)
  Q_PROPERTY(QColor backgroundColorHasSignedMe READ backgroundColorHasSignedMe WRITE setBackgroundColorHasSignedMe)
  Q_PROPERTY(QColor backgroundColorDenied READ backgroundColorDenied WRITE setBackgroundColorDenied)

public:
  /** Default Constructor */
  NetworkDialog(QWidget *parent = 0);

  virtual void updateDisplay() ; // overloaded from RsAutoUpdatePage

  QColor backgroundColorSelf() const { return mBackgroundColorSelf; }
  QColor backgroundColorOwnSign() const { return mBackgroundColorOwnSign; }
  QColor backgroundColorAcceptConnection() const { return mBackgroundColorAcceptConnection; }
  QColor backgroundColorHasSignedMe() const { return mBackgroundColorHasSignedMe; }
  QColor backgroundColorDenied() const { return mBackgroundColorDenied; }

  void setBackgroundColorSelf(QColor color) { PGPIdItemModel->setBackgroundColorSelf(color); mBackgroundColorSelf = color; }
  void setBackgroundColorOwnSign(QColor color) { PGPIdItemModel->setBackgroundColorOwnSign(color); mBackgroundColorOwnSign = color; }
  void setBackgroundColorAcceptConnection(QColor color) { PGPIdItemModel->setBackgroundColorAcceptConnection(color); mBackgroundColorAcceptConnection = color; }
  void setBackgroundColorHasSignedMe(QColor color) { PGPIdItemModel->setBackgroundColorHasSignedMe(color); mBackgroundColorHasSignedMe = color; }
  void setBackgroundColorDenied(QColor color) { PGPIdItemModel->setBackgroundColorDenied(color); mBackgroundColorDenied = color; }

protected:
  void changeEvent(QEvent *e);

private slots:

  void removeUnusedKeys() ;
  void makeFriend() ;
  void denyFriend() ;
  void peerdetails();
  void copyLink();

  /** Create the context popup menu and it's submenus */
  void connectTreeWidgetCostumPopupMenu( QPoint point );

  /** Called when user clicks "Load Cert" to choose location of a Cert file */

  //void on_actionAddFriend_activated();
  //void on_actionExportKey_activated();
  //void on_actionCreate_New_Profile_activated();
    
  void filterColumnChanged(int);

private:

  /* Color definitions (for standard see qss.default) */
  QColor mBackgroundColorSelf;
  QColor mBackgroundColorOwnSign;
  QColor mBackgroundColorAcceptConnection;
  QColor mBackgroundColorHasSignedMe;
  QColor mBackgroundColorDenied;

  RSTreeWidgetItemCompareRole *compareNetworkRole ;

  //iinternal long lived data
  std::list<RsPgpId> neighs;

  pgpid_item_model *PGPIdItemModel;
  pgpid_item_proxy *PGPIdItemProxy;

  /** Qt Designer generated object */
  Ui::NetworkDialog ui;
};

#endif

