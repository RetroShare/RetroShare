/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/SharedFilesDialog.h                     *
 *                                                                             *
 * Copyright (c) 2009 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef _SHAREDFILESDIALOG_H
#define _SHAREDFILESDIALOG_H

#include "ui_SharedFilesDialog.h"

#include <retroshare-gui/RsAutoUpdatePage.h>
#include "gui/RetroShareLink.h"
#include "util/RsProtectedTimer.h"

#include <set>

class RetroshareDirModel;
class QSortFilterProxyModel;

class SharedFilesDialog : public RsAutoUpdatePage
{
  Q_OBJECT

public:
  /** Default Constructor */
  SharedFilesDialog(RetroshareDirModel *tree_model,RetroshareDirModel *flat_model,QWidget *parent = 0);

  /** Default Destructor */
  ~SharedFilesDialog() {}

  virtual void hideEvent(QHideEvent *) ;
  virtual void showEvent(QShowEvent *) ;

protected:
  QTreeView *directoryView() ;
  virtual void showProperColumns() = 0 ;
  virtual bool isRemote() const = 0 ;

protected slots:
  virtual void spawnCustomPopupMenu(QPoint point) = 0;

private slots:
	/* For handling the model updates */
  void  preModDirectories(bool local) ;
  void  postModDirectories(bool local) ;

  /** Create the context popup menu and it's submenus */
//  void customPopupMenu(QPoint point) ;

  void copyLink();
  void copyLinkhtml();
  void sendLinkTo();
  void removeExtraFile();

  void collCreate();
  void collModif();
  void collView();
  void collOpen();

  void recommendFilesToMsg();
	
  void indicatorChanged(int index);

  void onFilterTextEdited();
  //void filterRegExpChanged();
  void clearFilter();
  void startFilter();

  void updateDirTreeView();

  public slots:
  void changeCurrentViewModel(int viewTypeIndex);
signals:
  void playFiles(QStringList files);

protected:
  /** Qt Designer generated object */
  Ui::SharedFilesDialog ui;
  virtual void processSettings(bool bLoad) = 0;

  void recursRestoreExpandedItems(const QModelIndex& index, const std::string& path, const std::set<std::string>& exp, const std::set<std::string>& vis, const std::set<std::string>& sel);
  void recursSaveExpandedItems(const QModelIndex& index, const std::string &path, std::set<std::string> &exp,std::set<std::string>& vis, std::set<std::string>& sel);
  void saveExpandedPathsAndSelection(std::set<std::string>& paths,std::set<std::string>& visible_indexes, std::set<std::string>& selected_indexes) ;
  void restoreExpandedPathsAndSelection(const std::set<std::string>& paths,const std::set<std::string>& visible_indexes, const std::set<std::string>& selected_indexes) ;
  void recursExpandAll(const QModelIndex& index);
  void expandAll();

protected:
  //now context menu are created again every time theu are called ( in some
  //slots.. Maybe it's not good...
  //** Define the popup menus for the Context menu */
  //QMenu* contextMnu;

  //QMenu* contextMnu2;

  void copyLinks(const QModelIndexList& lst, bool remote, QList<RetroShareLink>& urls, bool& has_unhashed_files);
  void copyLink (const QModelIndexList& lst, bool remote);

  void FilterItems();
  bool tree_FilterItem(const QModelIndex &index, const QString &text, int level);
  bool flat_FilterItem(const QModelIndex &index, const QString &text, int level);

  QModelIndexList getSelected();

  /** Defines the actions for the context menu for QTreeWidget */
  QAction* copylinkAct;
  QAction* sendlinkAct;
  QAction* sendchatlinkAct;
  QAction* copylinkhtmlAct;
  QAction* removeExtraFileAct;

  QAction *collCreateAct;
  QAction *collModifAct;
  QAction *collViewAct;
  QAction *collOpenAct;

  /* RetroshareDirModel */
  RetroshareDirModel *tree_model;
  RetroshareDirModel *flat_model;
  RetroshareDirModel *model;
  QSortFilterProxyModel *tree_proxyModel;
  QSortFilterProxyModel *flat_proxyModel;
  QSortFilterProxyModel *proxyModel;

  QString currentCommand;
  QString currentFile;

  QString lastFilterString;
  QString mLastFilterText ;
  RsProtectedTimer* mFilterTimer;
};

class LocalSharedFilesDialog : public SharedFilesDialog
{
	Q_OBJECT

	public:
		LocalSharedFilesDialog(QWidget *parent=NULL) ;
		virtual ~LocalSharedFilesDialog();

		virtual void spawnCustomPopupMenu(QPoint point);
		virtual void updatePage() { checkUpdate() ; }

	protected:
		virtual void processSettings(bool bLoad) ;
		virtual void showProperColumns() ;
		virtual bool isRemote() const { return false ; }

	private slots:
		void addShares();
		void checkUpdate() ;
		void playselectedfiles();
		void openfile();
		void openfolder();
		void runCommandForFile();
		void tryToAddNewAssotiation();
		void forceCheck();
  		void shareOnChannel();
  		void shareInForum();

		QAction* fileAssotiationAction(const QString fileName);

	private:
		QAction* openfileAct;
		QAction* openfolderAct;
		QAction* editshareAct;
};

class RemoteSharedFilesDialog : public SharedFilesDialog
{
	Q_OBJECT

	public:
		RemoteSharedFilesDialog(QWidget *parent=NULL) ;
		virtual ~RemoteSharedFilesDialog() ;

		virtual void spawnCustomPopupMenu(QPoint point);

	protected:
		virtual void processSettings(bool bLoad) ;
		virtual void showProperColumns() ;
		virtual bool isRemote() const { return true ; }

	private slots:
		void downloadRemoteSelected();
		void downloadRemoteSelectedInteractive();
		void expanded(const QModelIndex& indx);
};

#endif

