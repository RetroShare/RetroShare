/*******************************************************************************
 * gui/MessagesDialog.h                                                        *
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

#ifndef _MESSAGESDIALOG_H
#define _MESSAGESDIALOG_H

#include <QSortFilterProxyModel>

#include "mainpage.h"
#include "ui_MessagesDialog.h"

#define IMAGE_MESSAGES          ":/icons/png/messages.png"

class RSTreeWidgetItemCompareRole;
class MessageWidget;
class QTreeWidgetItem;
class RsMessageModel;

class MessagesDialog : public MainPage
{
  Q_OBJECT

  Q_PROPERTY(QColor textColorInbox READ textColorInbox WRITE setTextColorInbox)

public:
  /** Default Constructor */
  MessagesDialog(QWidget *parent = 0);
  /** Default Destructor */
  ~MessagesDialog();

  virtual QIcon iconPixmap() const { return QIcon(IMAGE_MESSAGES) ; } //MainPage
  virtual QString pageName() const { return tr("Mail") ; } //MainPage
  virtual QString helpText() const { return ""; } //MainPage

  virtual UserNotify *getUserNotify(QObject *parent);

// replaced by shortcut
//  virtual void keyPressEvent(QKeyEvent *) ;

  QColor textColorInbox() const { return mTextColorInbox; }

  void setTextColorInbox(QColor color) { mTextColorInbox = color; }

protected:
  bool eventFilter(QObject *obj, QEvent *ev);
  void changeEvent(QEvent *e);
  int getSelectedMessages(QList<QString>& mid);

public slots:
  //void insertMessages();
  void messagesTagsChanged();
  
private slots:
  /** Create the context popup menu and it's submenus */
  void messageTreeWidgetCustomPopupMenu(QPoint point);
  void folderlistWidgetCustomPopupMenu(QPoint);
  void showAuthorInPeopleTab();

  void changeBox(int newrow);
  void changeQuickView(int newrow);
  void updateCurrentMessage();
  void currentItemChanged(QTreeWidgetItem *item);
  void clicked(QTreeWidgetItem *item, int column);
  void doubleClicked(QTreeWidgetItem *item, int column);

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

  void updateInterface();

  void connectActions();

  void updateMessageSummaryList();
  void insertMsgTxtAndFiles(QTreeWidgetItem *item = NULL, bool bSetToRead = true);

  bool getCurrentMsg(std::string &cid, std::string &mid);
  void setMsgAsReadUnread(const QList<QTreeWidgetItem *> &items, bool read);
  void setMsgStar(const QList<QTreeWidgetItem *> &items, bool mark);

  int getSelectedMsgCount (QList<QTreeWidgetItem *> *items, QList<QTreeWidgetItem *> *itemsRead, QList<QTreeWidgetItem *> *itemsUnread, QList<QTreeWidgetItem *> *itemsStar);
  bool isMessageRead(QTreeWidgetItem *item);
  bool hasMessageStar(QTreeWidgetItem *item);

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
  int timerIndex;

  RSTreeWidgetItemCompareRole *mMessageCompareRole;
  MessageWidget *msgWidget;
  RsMessageModel *mMessageModel;
  QSortFilterProxyModel *mMessageProxyModel;

  /* Color definitions (for standard see qss.default) */
  QColor mTextColorInbox;

  /** Qt Designer generated object */
  Ui::MessagesDialog ui;
};

#endif

