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

#include <QStandardItemModel>
#include <QClipboard>

#include "DetailsDialog.h"
#include "TransfersDialog.h"

#include "retroshare/rsfiles.h"
#include "util/misc.h"
#include "FileTransferInfoWidget.h"
#include "RetroShareLink.h"

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

	//ui.commentsTreeView->setModel(CommentsModel);
	//ui.commentsTreeView->setSortingEnabled(true);
	//ui.commentsTreeView->setRootIsDecorated(false);

	/* Set header resize modes and initial section sizes */
	//QHeaderView * _coheader = ui.commentsTreeView->header();
	//_coheader->setResizeMode ( 0, QHeaderView::Custom);
	//_coheader->resizeSection ( 0, 100 );
	//_coheader->resizeSection ( 1, 240 );
	//_coheader->resizeSection ( 2, 100 );

	FileTransferInfoWidget *ftiw = new FileTransferInfoWidget();
	ui.fileTransferInfoWidget->setWidget(ftiw);
	ui.fileTransferInfoWidget->setWidgetResizable(true);
	ui.fileTransferInfoWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	ui.fileTransferInfoWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	ui.fileTransferInfoWidget->viewport()->setBackgroundRole(QPalette::NoRole);
	ui.fileTransferInfoWidget->setFrameStyle(QFrame::NoFrame);
	ui.fileTransferInfoWidget->setFocusPolicy(Qt::NoFocus);

	setAttribute(Qt::WA_DeleteOnClose,false) ;

	connect(ui.copylinkdetailsButton,SIGNAL(clicked()),this,SLOT(copyLink())) ;
}

void DetailsDialog::copyLink()
{
	QApplication::clipboard()->setText(	ui.Linktext->toPlainText() );
}
void DetailsDialog::on_ok_dButton_clicked()
{
    QDialog::hide();
}

void DetailsDialog::on_cancel_dButton_clicked()
{
    //reject();
    QDialog::hide();
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

void DetailsDialog::setFileHash(const std::string & hash) 
{
	dynamic_cast<FileTransferInfoWidget*>(ui.fileTransferInfoWidget->widget())->setFileHash(hash) ;

	FileInfo nfo ;
	if(!rsFiles->FileDetails(hash, RS_FILE_HINTS_DOWNLOAD, nfo)) 
		return ;

	RetroShareLink link ;
	link.createFile(QString::fromStdString(nfo.fname),nfo.size,QString::fromStdString(nfo.hash)) ;

	ui.Linktext->setText(link.toString()) ;
}

