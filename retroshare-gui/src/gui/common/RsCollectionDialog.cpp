/****************************************************************
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

#include <QCheckBox>
#include <QMessageBox>
#include <QDir>
#include <QKeyEvent>
#include "RsCollectionDialog.h"
#include "RsCollectionFile.h"
#include "util/misc.h"

RsCollectionDialog::RsCollectionDialog(const QString& CollectionFileName,const std::vector<RsCollectionFile::DLinfo>& dlinfos)
	: _dlinfos(dlinfos),_filename(CollectionFileName)
{
	setupUi(this) ;

	setWindowFlags(Qt::Window); // for maximize button
	setWindowFlags(windowFlags() & ~Qt::WindowMinimizeButtonHint);

	setWindowTitle(QString("%1 - %2").arg(windowTitle()).arg(QFileInfo(_filename).completeBaseName()));

	// 1 - add all elements to the list.

	_fileEntriesTW->setColumnCount(3) ;

	QTreeWidgetItem *headerItem = _fileEntriesTW->headerItem();
	headerItem->setText(0, tr("File"));
	headerItem->setText(1, tr("Size"));
	headerItem->setText(2, tr("Hash"));

	uint32_t size = dlinfos.size();

	uint64_t total_size ;
	uint32_t total_files ;

	for(uint32_t i=0;i<size;++i)
	{
		const RsCollectionFile::DLinfo &dlinfo = dlinfos[i];

		QTreeWidgetItem *item = new QTreeWidgetItem;

		item->setFlags(Qt::ItemIsUserCheckable | item->flags());
		item->setCheckState(0, Qt::Checked);
		item->setData(0, Qt::UserRole, i);
		item->setText(0, dlinfo.path + "/" + dlinfo.name);
		item->setText(1, misc::friendlyUnit(dlinfo.size));
		item->setText(2, dlinfo.hash);

		_fileEntriesTW->addTopLevelItem(item);

		total_size += dlinfo.size ;
		total_files++ ;
	}

	_filename_TL->setText(_filename) ;
	for (int column = 0; column < _fileEntriesTW->columnCount(); ++column) {
		_fileEntriesTW->resizeColumnToContents(column);
	}

	updateSizes() ;

	// 2 - connect necessary signals/slots

	connectUpdate(true);
	connect(_selectAll_PB,SIGNAL(clicked()),this,SLOT(selectAll())) ;
	connect(_deselectAll_PB,SIGNAL(clicked()),this,SLOT(deselectAll())) ;
	connect(_cancel_PB,SIGNAL(clicked()),this,SLOT(cancel())) ;
	connect(_download_PB,SIGNAL(clicked()),this,SLOT(download())) ;

	_fileEntriesTW->installEventFilter(this);
}

bool RsCollectionDialog::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == _fileEntriesTW) {
		if (event->type() == QEvent::KeyPress) {
			QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
			if (keyEvent && keyEvent->key() == Qt::Key_Space) {
				// Space pressed

				// get state of current item
				QTreeWidgetItem *item = _fileEntriesTW->currentItem();
				if (item) {
					Qt::CheckState checkState = (item->checkState(0) == Qt::Checked) ? Qt::Unchecked : Qt::Checked;

					connectUpdate(false);

					// set state of all selected items
					QList<QTreeWidgetItem*> selectedItems = _fileEntriesTW->selectedItems();
					QList<QTreeWidgetItem*>::iterator it;
					for (it = selectedItems.begin(); it != selectedItems.end(); ++it) {
						(*it)->setCheckState(0, checkState);
					}

					updateSizes();

					connectUpdate(true);
				}

				return true; // eat event
			}
		}
	}
	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}

void RsCollectionDialog::connectUpdate(bool doConnect)
{
	if (doConnect) {
		connect(_fileEntriesTW, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*,int)));
	} else {
		disconnect(_fileEntriesTW, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(itemChanged(QTreeWidgetItem*,int)));
	}
}

void RsCollectionDialog::updateSizes()
{
	uint64_t total_size = 0 ;
	uint32_t total_files = 0 ;

	QTreeWidgetItemIterator itemIterator(_fileEntriesTW);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		if (item->checkState(0) == Qt::Checked) {
			total_size += _dlinfos[item->data(0, Qt::UserRole).toInt()].size ;
			++total_files ;
		}
	}
	_selectedFiles_TL->setText(QString::number(total_files)) ;
	_totalSize_TL->setText(misc::friendlyUnit(total_size)) ;
}

void RsCollectionDialog::itemChanged(QTreeWidgetItem */*item*/, int /*column*/)
{
	updateSizes();
}

void RsCollectionDialog::selectDeselectAll(bool select)
{
	connectUpdate(false);

	QTreeWidgetItemIterator itemIterator(_fileEntriesTW);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;
		item->setCheckState(0, select ? Qt::Checked : Qt::Unchecked);
	}

	updateSizes();

	connectUpdate(true);
}

void RsCollectionDialog::selectAll()
{
	std::cerr << "Selecting all !" << std::endl;

	selectDeselectAll(true);
}

void RsCollectionDialog::deselectAll()
{
	std::cerr << "Deselecting all !" << std::endl;

	selectDeselectAll(false);
}

void RsCollectionDialog::cancel() 
{
	std::cerr << "Canceling!" << std::endl;
	close() ;
}

void RsCollectionDialog::download() 
{
	std::cerr << "Downloading!" << std::endl;

	QString dldir = QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()) ;

	std::cerr << "downloading all these files:" << std::endl;

	QTreeWidgetItemIterator itemIterator(_fileEntriesTW);
	QTreeWidgetItem *item;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		const RsCollectionFile::DLinfo &dlinfo = _dlinfos[item->data(0, Qt::UserRole).toInt()];

		if (item->checkState(0) == Qt::Checked) {
			std::cerr << dlinfo.name.toStdString() << " " << dlinfo.hash.toStdString() << " " << dlinfo.size << " " << dlinfo.path.toStdString() << std::endl;
			QString cleanPath = dldir + dlinfo.path ;
			std::cerr << "making directory " << cleanPath.toStdString() << std::endl;

			if(!QDir(QApplication::applicationDirPath()).mkpath(cleanPath))
				QMessageBox::warning(NULL,QObject::tr("Unable to make path"),QObject::tr("Unable to make path:")+"<br>  "+cleanPath) ;

			rsFiles->FileRequest(dlinfo.name.toUtf8().constData(), dlinfo.hash.toUtf8().constData(), dlinfo.size, cleanPath.toUtf8().constData(), RS_FILE_REQ_ANONYMOUS_ROUTING, std::list<std::string>());
		}
		else
			std::cerr<<"Skipping file : " << dlinfo.name.toStdString() << std::endl;
	}

	close();
}
