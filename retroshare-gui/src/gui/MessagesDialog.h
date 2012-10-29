/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef _MESSAGESDIALOG_H
#define _MESSAGESDIALOG_H

#include <QSortFilterProxyModel>

#include "mainpage.h"
#include "ui_MessagesDialog.h"

class MessageWidget;

class MessagesDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  MessagesDialog(QWidget *parent = 0);
  /** Default Destructor */
  ~MessagesDialog();

  virtual UserNotify *getUserNotify(QObject *parent);

// replaced by shortcut
//  virtual void keyPressEvent(QKeyEvent *) ;

protected:
  bool eventFilter(QObject *obj, QEvent *ev);

public slots:
  void insertMessages();
  void messagesTagsChanged();
  
private slots:
  /** Create the context popup menu and it's submenus */
  void messageslistWidgetCostumPopupMenu( QPoint point );
  void folderlistWidgetCostumPopupMenu(QPoint);

  void changeBox(int newrow);
  void changeQuickView(int newrow);
  void updateCurrentMessage();
  void currentChanged(const QModelIndex&);
  void clicked(const QModelIndex&);
  void doubleClicked(const QModelIndex &);

  void newmessage();
  void openAsWindow();
  void openAsTab();
  void editmessage();

  void removemessage();
  void undeletemessage();

  void markAsRead();
  void markAsUnread();
  void markWithStar(bool checked);

  void emptyTrash();

  void buttonStyle();
  
  void filterChanged(const QString &text);
  void filterColumnChanged(int column);
  
  void tagAboutToShow();
  void tagSet(int tagId, bool set);
  void tagRemoveAll();

  void tabChanged(int tab);
  void tabCloseRequested(int tab);

  void updateInterface();

private:
  class LockUpdate
  {
  public:
      LockUpdate (MessagesDialog *pDialog, bool bUpdate);
      ~LockUpdate ();

      void setUpdate(bool bUpdate);

  private:
      MessagesDialog *m_pDialog;
      bool m_bUpdate;
  };

  class QStandardItemModel *MessagesModel;
  QSortFilterProxyModel *proxyModel;

  void connectActions();

  void updateMessageSummaryList();
  void insertMsgTxtAndFiles(QModelIndex index = QModelIndex(), bool bSetToRead = true);

  bool getCurrentMsg(std::string &cid, std::string &mid);
  void setMsgAsReadUnread(const QList<int> &Rows, bool read);
  void setMsgStar(const QList<int> &Rows, bool mark);

  int getSelectedMsgCount (QList<int> *pRows, QList<int> *pRowsRead, QList<int> *pRowsUnread, QList<int> *pRowsStar);
  bool isMessageRead(int nRow);
  bool hasMessageStar(int nRow);

  void processSettings(bool load);

  void setToolbarButtonStyle(Qt::ToolButtonStyle style);
  void fillQuickView();

  void closeTab(const std::string &msgId);

  bool inProcessSettings;
  bool inChange;
  int lockUpdate; // use with LockUpdate

  enum { LIST_NOTHING, LIST_BOX, LIST_QUICKVIEW } listMode;

  std::string mCurrMsgId;

  // timer and index for showing message
  QTimer *timer;
  QModelIndex timerIndex;

  MessageWidget *msgWidget;

  /** Qt Designer generated object */
  Ui::MessagesDialog ui;
};

#endif

