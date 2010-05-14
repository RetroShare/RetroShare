/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006-2010,  RetroShare Team
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

#include "DetailsDialog.h"

#include <QAction>
#include <QTreeView>
#include <QList>
#include <QtDebug>
#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QPixmap>
#include <QHeaderView>
#include <QModelIndex>
#include <QStandardItemModel>

#include "util/misc.h"

#include "TransfersDialog.h"

/** Default constructor */
DetailsDialog::DetailsDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);

 
    setAttribute ( Qt::WA_DeleteOnClose, true );

    CommentsModel = new QStandardItemModel(0, 3);
    CommentsModel->setHeaderData(0, Qt::Horizontal, tr("Rating"));
    CommentsModel->setHeaderData(1, Qt::Horizontal, tr("Comments"));
    CommentsModel->setHeaderData(2, Qt::Horizontal, tr("File Name"));
    
    ui.commentsTreeView->setModel(CommentsModel);
    ui.commentsTreeView->setSortingEnabled(true);
    ui.commentsTreeView->setRootIsDecorated(false);
    
  /* Set header resize modes and initial section sizes */
	QHeaderView * _coheader = ui.commentsTreeView->header();
	_coheader->setResizeMode ( 0, QHeaderView::Custom);
  _coheader->resizeSection ( 0, 100 );
	_coheader->resizeSection ( 1, 240 );
	_coheader->resizeSection ( 2, 100 );
	
}

/** Destructor. */
DetailsDialog::~DetailsDialog()
{
    TransfersDialog::detailsdlg = NULL;
}

void DetailsDialog::on_ok_dButton_clicked()
{
    QDialog::close();
}

void DetailsDialog::on_cancel_dButton_clicked()
{
    //reject();
    QDialog::close();
}


void
DetailsDialog::show()
{
    ui.tabWidget->setCurrentIndex(0);
    if (!this->isVisible()) {
        QDialog::show();
    } else {
        QDialog::activateWindow();
        setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
        QDialog::raise();
    }
}

void DetailsDialog::closeEvent (QCloseEvent * event)
{
  QWidget::closeEvent(event);
}

void DetailsDialog::setFileName(const QString & filename) 
{
    int c;
    QModelIndex index;
    
    c = CommentsModel->rowCount();
    CommentsModel->removeRows(0,c);
    
    c = CommentsModel->rowCount();
    CommentsModel->insertRow(c);
    
    index = CommentsModel->index(c, 0);
    CommentsModel->setData(index, tr("Not Rated"));
    
    index = CommentsModel->index(c, 1);
    CommentsModel->setData(index, tr("No Comments"));
    
    index = CommentsModel->index(c, 2);
    CommentsModel->setData(index, filename);
    
    ui.name_label_2->setText(filename);
    
}

void DetailsDialog::setHash(const QString & hash) 
{
	ui.hash_label_2->setText(hash);
}

void DetailsDialog::setSize(const qulonglong & size) 
{
	ui.size_label_2->setText(misc::friendlyUnit(size) + " " + "(" + QString::number(size) + " " + "Bytes" + ")");
}

void DetailsDialog::setStatus(const QString & status) 
{
	ui.status_label_2->setText(status);
}

void DetailsDialog::setPriority(const QString & priority) 
{
	ui.priority_label_2->setText(priority);
}

void DetailsDialog::setType(const QString & type) 
{
	ui.type_label_2->setText(type);
}

void DetailsDialog::setSources(const QString & sources) 
{
	ui.sources_line->setText(sources);
}

void DetailsDialog::setDatarate(const double & datarate) 
{
      QString temp;
			temp.clear();
			temp.sprintf("%.2f", datarate/1024.);
			temp += " KB/s";

      ui.datarate_line->setText(temp);
}

void DetailsDialog::setCompleted(const QString & completed)
{
        ui.completed_line->setText(completed);
}

void DetailsDialog::setRemaining(const QString & remaining)
{
        ui.remaining_line->setText(remaining);
}

void DetailsDialog::setDownloadtime(const QString & downloadtime)
{
        ui.downloadtime_line->setText(downloadtime);
}

void DetailsDialog::setLink(const QString & link) 
{
	ui.Linktext->setText(link);
}

void DetailsDialog::setChunkSize(uint32_t chunksize)
{
    ui.chunksizelabel->setText(misc::friendlyUnit(chunksize));
}

void DetailsDialog::setNumberOfChunks(size_t numberofchunks)
{
    ui.numberofchunkslabel->setText(QString::number(numberofchunks));
}
