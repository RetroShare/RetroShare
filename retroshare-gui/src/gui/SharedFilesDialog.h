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
  void  preModDirectories() ;
  void  postModDirectories() ;

  /** Create the context popup menu and it's submenus */
//  void customPopupMenu(QPoint point) ;

  void copyLink();
  void copyLinkhtml();
  void sendLinkTo();
#ifdef RS_USE_LINKS
  void sendLinkToCloud();
  void addLinkToCloud();
#endif

//==  void showFrame(bool show);
//==  void showFrameRemote(bool show);
//==  void showFrameSplitted(bool show);

  void recommendFilesToMsg();
	
  void indicatorChanged(int index);

  void filterRegExpChanged();
  void clearFilter();
  void startFilter();

  public slots:
	  void changeCurrentViewModel(int) ;
signals:
  void playFiles(QStringList files);

protected:
  /** Qt Designer generated object */
  Ui::SharedFilesDialog ui;
  virtual void processSettings(bool bLoad) = 0;

protected:
  //now context menu are created again every time theu are called ( in some
  //slots.. Maybe it's not good...
  //** Define the popup menus for the Context menu */
  //QMenu* contextMnu;

  //QMenu* contextMnu2;

  void copyLink (const QModelIndexList& lst, bool remote);

  void FilterItems();
  bool tree_FilterItem(const QModelIndex &index, const QString &text, int level);
  bool flat_FilterItem(const QModelIndex &index, const QString &text, int level);

  QModelIndexList getSelected();

  /** Defines the actions for the context menu for QTreeWidget */
  QAction* copylinkAct;
  QAction* sendlinkAct;
#ifdef RS_USE_LINKS
  QAction* sendlinkCloudAct;
  QAction* addlinkCloudAct;
#endif
  QAction* sendchatlinkAct;
  QAction* copylinkhtmlAct;

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
		void createCollectionFile();
		void checkUpdate() ;
		void editSharePermissions();
		void playselectedfiles();
		void openfile();
		void openfolder();
		void runCommandForFile();
		void tryToAddNewAssotiation();
		void forceCheck();

		QAction* fileAssotiationAction(const QString fileName);

	private:
		QAction* openfileAct;
		QAction* createcollectionfileAct;
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
};

#endif

