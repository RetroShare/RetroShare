/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#ifndef _TRANSFERSDIALOG_H
#define _TRANSFERSDIALOG_H

#include <QFileDialog>
#include <QProgressBar>
#include <QtGui>
#include <QObject>
#include <QModelIndex>
#include <QVariant>
#include <stdint.h>

#include "mainpage.h"
#include "ui_TransfersDialog.h"


class DLListDelegate;
class ULListDelegate;
class QStandardItemModel;

class TransfersDialog : public MainPage
{
	Q_OBJECT

	public:
		/** Default Constructor */
		TransfersDialog(QWidget *parent = 0);
		/** Default Destructor */
		~TransfersDialog();

		virtual void keyPressEvent(QKeyEvent *) ;

	public slots:
		void insertTransfers();

		void handleDownloadRequest(const QString& url);

	private slots:

		/** Create the context popup menu and it's submenus */
		void downloadListCostumPopupMenu( QPoint point );

		void cancel();
		/** removes finished Downloads**/
		void clearcompleted();
		void playSelectedTransfer();

		void copyLink();
    void pasteLink();

    void rootdecorated();
    void rootisnotdecorated();

    void pauseFileTransfer();
    void resumeFileTransfer();
    void openFolderTransfer();
    void openTransfer();
    void previewTransfer();

    /** clear download or all queue - for pending dwls */
    void clearQueuedDwl();
    void clearQueue();

    /** modify download priority actions */
    void priorityLow();
	void priorityNormal();
	void priorityHigh();
	void priorityAuto();

	/** save sort indicator for next transfers display */
	void saveSortIndicator(int logicalIndex, Qt::SortOrder order);

	signals:
		void playFiles(QStringList files);

	private:
		QString getPeerName(const std::string& peer_id) const ;

		QStandardItemModel *DLListModel;
		QStandardItemModel *ULListModel;
		QItemSelectionModel *selection;
		QItemSelectionModel *selectionup;

		DLListDelegate *DLDelegate;
		ULListDelegate *ULDelegate;
		qlonglong fileSize;
		double progress;
		double dlspeed;
		QString status, icon, name;
		qlonglong completed, remaining;

		int _sortCol;
		Qt::SortOrder _sortOrder;


		/** Create the actions on the tray menu or menubar */
		void createActions();

		/** Define the popup menus for the Context menu */
		QMenu* contextMnu;
		/** Defines the actions for the context menu */
		QAction* showdowninfoAct;
		QAction* cancelAct;
		QAction* clearcompletedAct;
		QAction* copylinkAct;
    QAction* pastelinkAct;
    QAction* rootisnotdecoratedAct;
    QAction* rootisdecoratedAct;
		QAction *pauseAct;
		QAction *resumeAct;
		QAction *openfolderAct;
		QAction *openfileAct;
		QAction *previewfileAct;
		QAction *clearQueuedDwlAct;
		QAction *clearQueueAct;
		QAction *changePriorityAct;
		QAction *priorityLowAct;
		QAction *priorityNormalAct;
		QAction *priorityHighAct;
		QAction *priorityAutoAct;

	void getIdOfSelectedItems(QList<QStandardItem *>& items);
    bool controlTransferFile(uint32_t flags);
    void changePriority(int priority);

		QTreeView *downloadList;

		/** Adds a new action to the toolbar. */
		void addAction(QAction *action, const char *slot = 0);

		/** Qt Designer generated object */
		Ui::TransfersDialog ui;

		public slots:
			int addItem(QString symbol, QString name, QString coreID, qlonglong size, double progress, double dlspeed, QString sources, QString status, QString priority, qlonglong completed, qlonglong remaining);
			bool addPeerToItem(int row, QString symbol, QString name, QString coreID, qlonglong fileSize, double progress, double dlspeed, QString sources, QString status, qlonglong completed, qlonglong remaining);
		void delItem(int row);

		int addUploadItem(QString symbol, QString name, QString coreID, qlonglong size, double progress, double dlspeed, QString sources, QString status, qlonglong completed, qlonglong remaining);
		void delUploadItem(int row);

		void editItem(int row, int column, QVariant data);
		void updateProgress(int value);

		double getProgress(int row, QStandardItemModel *model);
		double getSpeed(int row, QStandardItemModel *model);
		QString getFileName(int row, QStandardItemModel *model);
		QString getStatus(int row, QStandardItemModel *model);
		QString getID(int row, QStandardItemModel *model);
		QString getPriority(int row, QStandardItemModel *model);
		qlonglong getFileSize(int row, QStandardItemModel *model);
		qlonglong getTransfered(int row, QStandardItemModel *model);
		qlonglong getRemainingTime(int row, QStandardItemModel *model);
};

#endif

