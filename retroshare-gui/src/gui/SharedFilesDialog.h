/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006-2009, RetroShare Team
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

#include "RsAutoUpdatePage.h"
#include "ui_SharedFilesDialog.h"

class RetroshareDirModel;
class QSortFilterProxyModel;

class SharedFilesDialog : public RsAutoUpdatePage
{
  Q_OBJECT

public:
  /** Default Constructor */
  SharedFilesDialog(QWidget *parent = 0);
  /** Default Destructor */
  ~SharedFilesDialog();

  virtual void updatePage() { checkUpdate() ; }
  virtual void hideEvent(QHideEvent *) ;
  virtual void showEvent(QShowEvent *) ;


private slots:

	/* For handling the model updates */
  void  preModDirectories(bool update_local);
  void  postModDirectories(bool update_local);

  void checkUpdate();
  void forceCheck();

  /** Create the context popup menu and it's submenus */
  void shareddirtreeviewCostumPopupMenu( QPoint point );

  void sharedDirTreeWidgetContextMenu( QPoint point );

  void downloadRemoteSelected();
  void createCollectionFile();
  void addMsgRemoteSelected();

  void copyLinkRemote();
  void copyLinkLocal();
  void copyLinkhtml();
  void sendLinkTo();
  void sendremoteLinkTo();
#ifdef RS_USE_LINKS
  void sendLinkToCloud();
  void addLinkToCloud();
#endif

  void showFrame(bool show);
  void showFrameRemote(bool show);
  void showFrameSplitted(bool show);

  void playselectedfiles();
  void openfile();
  void openfolder();
  void editSharePermissions();

  void recommendFilesToMsg();
  void runCommandForFile();
  void tryToAddNewAssotiation();
	
  void indicatorChanged(int index);

  void filterRegExpChanged();
  void clearFilter();
  void startFilter();

  public slots:
	  void changeCurrentViewModel(int) ;
signals:
  void playFiles(QStringList files);

private:
  //now context menu are created again every time theu are called ( in some
  //slots.. Maybe it's not good...
  //** Define the popup menus for the Context menu */
  //QMenu* contextMnu;

  //QMenu* contextMnu2;

  void processSettings(bool bLoad);

  void copyLink (const QModelIndexList& lst, bool remote);

  void FilterItems();
  bool tree_FilterItem(const QModelIndex &index, const QString &text, int level);
  bool flat_FilterItem(const QModelIndex &index, const QString &text, int level);

  QModelIndexList getRemoteSelected();
  QModelIndexList getLocalSelected();

  /** Defines the actions for the context menu for QTreeWidget */
  QAction* openfileAct;
  QAction* createcollectionfileAct;
  QAction* openfolderAct;
  QAction* copyremotelinkAct;
  QAction* copylinklocalAct;
  QAction* sendlinkAct;
  QAction* editshareAct;
#ifdef RS_USE_LINKS
  QAction* sendlinkCloudAct;
  QAction* addlinkCloudAct;
#endif
  QAction* sendchatlinkAct;
  QAction* copylinklocalhtmlAct;

  /** Qt Designer generated object */
  Ui::SharedFilesDialog ui;

  /* RetroshareDirModel */
  RetroshareDirModel *tree_model;
  RetroshareDirModel *flat_model;
  RetroshareDirModel *model;
  QSortFilterProxyModel *tree_proxyModel;
  QSortFilterProxyModel *flat_proxyModel;
  QSortFilterProxyModel *proxyModel;

  RetroshareDirModel *localModel;
  QSortFilterProxyModel *localProxyModel;

  QString currentCommand;
  QString currentFile;

  QString lastFilterString;

  QAction* fileAssotiationAction(const QString fileName);
};

#endif

