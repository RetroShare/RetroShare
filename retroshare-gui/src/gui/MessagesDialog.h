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

#include <list>
#include <QFileDialog>
#include <QModelIndex>
#include <QSortFilterProxyModel>

#include <QVariant>

#include <retroshare/rsmsgs.h>
#include "mainpage.h"
#include "ui_MessagesDialog.h"

#include "settings/NewTag.h"

class RSettings;

class MessagesDialog : public MainPage 
{
  Q_OBJECT

public:
  /** Default Constructor */
  MessagesDialog(QWidget *parent = 0);
  /** Default Destructor */
  ~MessagesDialog();

  static void initStandardTagItems(std::map<int, TagItem> &Items);
  void getTagItems(std::map<int, TagItem> &Items);
  void setTagItems(std::map<int, TagItem> &Items);

// replaced by shortcut
//  virtual void keyPressEvent(QKeyEvent *) ;

public slots:
  void insertMessages();

  
private slots:

  /** Create the context popup menu and it's submenus */
  void messageslistWidgetCostumPopupMenu( QPoint point ); 
  void msgfilelistWidgetCostumPopupMenu(QPoint);  

  void changeBox( int newrow );
  void changeTag( int newrow );
  void updateCurrentMessage();
  void currentChanged(const QModelIndex&);
  void clicked(const QModelIndex&);
  void doubleClicked(const QModelIndex &);

  void newmessage();
  void editmessage();

  void replytomessage();
  void replyallmessage();
  void forwardmessage();

  void print();
  void printpreview();
  
  bool fileSave();
  bool fileSaveAs();
  
  void removemessage();
  void undeletemessage();

  void markAsRead();
  void markAsUnread();

  void anchorClicked (const QUrl &);
  
  void getcurrentrecommended();
//  void getallrecommended();

  /* handle splitter */
  void togglefileview();
  
  void buttonstextbesideicon();
  void buttonsicononly();
  void buttonstextundericon();
  
  void loadToolButtonsettings();
  
  void filterRegExpChanged();
  void filterColumnChanged();
  
  void clearFilter();
  void tagTriggered(QAction *pAction);
  void tagAboutToShow();

private:
  class LockUpdate
  {
  public:
      LockUpdate (MessagesDialog *pDialog, bool bUpdate);
      ~LockUpdate ();

  private:
      MessagesDialog *m_pDialog;
      bool m_bUpdate;
  };

  class QStandardItemModel *MessagesModel;
  QSortFilterProxyModel *proxyModel;

  void updateMessageSummaryList();
  void insertMsgTxtAndFiles(QModelIndex index = QModelIndex(), bool bSetToRead = true);

  bool getCurrentMsg(std::string &cid, std::string &mid);
  void setMsgAsReadUnread(const QList<int> &Rows, bool bRead);

  void setCurrentFileName(const QString &fileName);

  int getSelectedMsgCount (QList<int> *pRows, QList<int> *pRowsRead, QList<int> *pRowsUnread);
  bool isMessageRead(int nRow);

  /* internal handle splitter */
  void togglefileview_internal();

  void processSettings(bool bLoad);

  void setToolbarButtonStyle(Qt::ToolButtonStyle style);
  void fillTags();
  bool m_bProcessSettings;
  bool m_bInChange;
  int m_nLockUpdate; // use with LockUpdate

  enum { LIST_NOTHING, LIST_BOX, LIST_TAG } m_eListMode;

  std::string mCurrCertId;
  std::string mCurrMsgId;

  QString fileName;
  QFont mFont;
  RSettings *m_pConfig;

  // timer and index for showing message
  QTimer *timer;
  QModelIndex timerIndex;

  /** Qt Designer generated object */
  Ui::MessagesDialog ui;
};

#endif

