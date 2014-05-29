/*************************************:***************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 - 2011 RetroShare Team
 *
 *  Cyril Soler (csoler@users.sourceforge.net)
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

#include "ui_RsCollectionDialog.h"
#include "RsCollectionFile.h"
#include <QFileSystemModel>
#include <QSortFilterProxyModel>

class QCheckBox ;

class RsCollectionDialog: public QDialog
{
	Q_OBJECT

public:
	RsCollectionDialog(const QString& filename
	                   , const std::vector<ColFileInfo> &colFileInfos
	                   , const bool& creation
	                   , const bool& readOnly = false) ;
	virtual ~RsCollectionDialog();

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

private slots:
	void directoryLoaded(QString dirLoaded);
	void updateSizes() ;
	void changeFileName() ;
	void add() ;
	void addRecursive() ;
	void remove() ;
	void processItem(QMap<QString, QString> &dirToAdd
	                 , int &index
                   , ColFileInfo &parent
                   ) ;
	void makeDir() ;
	void fileHashingFinished(QList<HashedFile> hashedFiles) ;
	void itemChanged(QTreeWidgetItem* item,int col) ;
	void cancel() ;
	void download() ;
	void save() ;

signals:
	void saveColl(std::vector<ColFileInfo>, QString);

private:
	void processSettings(bool bLoad) ;
	QTreeWidgetItem*  getRootItem();
	bool updateList();
	bool addChild(QTreeWidgetItem *parent, const std::vector<ColFileInfo> &child);
	void addRecursive(bool recursive) ;
	bool addAllChild(QFileInfo &fileInfoParent
	                 , QMap<QString, QString > &dirToAdd
	                 , QStringList &fileToHash
	                 , int &count);
	void saveChild(QTreeWidgetItem *parentItem, ColFileInfo *parentInfo = NULL);

	Ui::RsCollectionDialog ui;
	QString _fileName ;
	const bool _creationMode ;
	const bool _readOnly;
	std::vector<ColFileInfo> _newColFileInfos ;

	QFileSystemModel *_dirModel;
	QSortFilterProxyModel *_tree_proxyModel;
	QItemSelectionModel *_selectionProxy;
	bool _dirLoaded;
	QHash<QString,QString> _listOfFilesAddedInDir;
};
