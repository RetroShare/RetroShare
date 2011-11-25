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

RsCollectionDialog::RsCollectionDialog(const QString& CollectionFileName,const std::vector<RsCollectionFile::DLinfo>& dlinfos)
	: _dlinfos(dlinfos),_filename(CollectionFileName)
{
	setupUi(this) ;

	setWindowTitle(QString("%1 - %2").arg(windowTitle()).arg(QFileInfo(_filename).baseName()));

	// 1 - add all elements to the list.

	int row = 0;
	_fileEntriesTW->setColumnCount(4) ;

	_fileEntriesTW->setHorizontalHeaderItem(0,new QTableWidgetItem(QString())) ;
	_fileEntriesTW->setHorizontalHeaderItem(1,new QTableWidgetItem(tr("File"))) ;
	_fileEntriesTW->setHorizontalHeaderItem(2,new QTableWidgetItem(tr("Size"))) ;
	_fileEntriesTW->setHorizontalHeaderItem(3,new QTableWidgetItem(tr("Hash"))) ;

	QHeaderView *header = _fileEntriesTW->horizontalHeader();
	header->setResizeMode(0, QHeaderView::Fixed);

	header->setHighlightSections(false);

	_cboxes.clear() ;
	_cboxes.resize(dlinfos.size(),NULL) ;
	uint64_t total_size ;
	uint32_t total_files ;

	for(uint32_t i=0;i<dlinfos.size();++i)
	{
		_fileEntriesTW->insertRow(row) ;

		QWidget *widget = new QWidget;

		QCheckBox *cb = new QCheckBox(widget);
		cb->setChecked(true) ;

		QHBoxLayout *layout = new QHBoxLayout(widget);
		layout->addWidget(cb, 0, Qt::AlignCenter);
		layout->setSpacing(0);
		layout->setContentsMargins(10, 0, 0, 0); // to be centered
		widget->setLayout(layout);

		connect(cb,SIGNAL(toggled(bool)),this,SLOT(updateSizes())) ;

		_cboxes[i] = cb ;

		_fileEntriesTW->setCellWidget(row,0,widget) ;
		_fileEntriesTW->setItem(row,1,new QTableWidgetItem(dlinfos[i].path + "/" + dlinfos[i].name)) ;
		_fileEntriesTW->setItem(row,2,new QTableWidgetItem(QString::number(dlinfos[i].size))) ;
		_fileEntriesTW->setItem(row,3,new QTableWidgetItem(dlinfos[i].hash)) ;

		total_size += dlinfos[i].size ;
		total_files++ ;

		++row  ;
	}

	_filename_TL->setText(_filename) ;
	_fileEntriesTW->resizeColumnsToContents() ;

	updateSizes() ;

	// 2 - connect necessary signals/slots
	
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
				QModelIndex currentIndex = _fileEntriesTW->currentIndex();
				QModelIndex index = _fileEntriesTW->model()->index(currentIndex.row(), 0, currentIndex.parent());
				QWidget *widget = dynamic_cast<QWidget*>(_fileEntriesTW->indexWidget(index));
				if (widget) {
					QCheckBox *cb = dynamic_cast<QCheckBox*>(widget->children().front());
					if (cb) {
						cb->toggle();
					}
				}

				return true; // eat event
			}
		}
	}
	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}

void RsCollectionDialog::updateSizes()
{
	uint64_t total_size = 0 ;
	uint32_t total_files = 0 ;

	for(size_t i=0;i<_dlinfos.size();++i)
		if(_cboxes[i]->isChecked())
		{
			total_size += _dlinfos[i].size ;
			++total_files ;
		}
	_selectedFiles_TL->setText(QString::number(total_files)) ;
	_totalSize_TL->setText(QString::number(total_size)) ;
}

void RsCollectionDialog::selectAll() const
{
	std::cerr << "Selecting all !" << std::endl;
	for(size_t i=0;i<_dlinfos.size();++i)
		_cboxes[i]->setChecked(true) ;
}

void RsCollectionDialog::deselectAll() const
{
	std::cerr << "Deselecting all !" << std::endl;
	for(size_t i=0;i<_dlinfos.size();++i)
		_cboxes[i]->setChecked(false) ;
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

	for(uint32_t i=0;i<_dlinfos.size();++i)
		if(_cboxes[i]->isChecked()) 
		{
			std::cerr << _dlinfos[i].name.toStdString() << " " << _dlinfos[i].hash.toStdString() << " " << _dlinfos[i].size << " " << _dlinfos[i].path.toStdString() << std::endl;
			QString cleanPath = dldir + _dlinfos[i].path ;
			std::cerr << "making directory " << cleanPath.toStdString() << std::endl;

			if(!QDir(cleanPath).mkpath(cleanPath))
				QMessageBox::warning(NULL,QObject::tr("Unable to make path"),QObject::tr("Unable to make path:")+"<br>  "+cleanPath) ;

			rsFiles->FileRequest(_dlinfos[i].name.toUtf8().constData(), _dlinfos[i].hash.toUtf8().constData(), _dlinfos[i].size, cleanPath.toUtf8().constData(), RS_FILE_HINTS_NETWORK_WIDE, std::list<std::string>());
		}
		else
			std::cerr<<"Skipping file : " << _dlinfos[i].name.toStdString() << std::endl;

	close();
}
