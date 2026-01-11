/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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

#include "ProfileView.h"
#include "ProfileEdit.h"
#include "gui/common/AvatarDefs.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmail.h>
#include <retroshare/rschats.h>

//#include <QContextMenuEvent>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>


/** Constructor */
ProfileView::ProfileView(QWidget *parent)
: QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.photoLabel, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( imageCustomPopupMenu( QPoint ) ) );
  connect( ui.profileTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( profileCustomPopupMenu( QPoint ) ) );
  connect( ui.fileTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( fileCustomPopupMenu( QPoint ) ) );
  //

  // connect up the buttons.
  connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(closeView()));
  connect(ui.profileditButton, SIGNAL(clicked()), this, SLOT(profileEdit()));

  loadAvatar();
}

/** context popup menus */
void ProfileView::imageCustomPopupMenu( QPoint /*point*/ )
{
      if (!mIsOwnId)
      {
      	return;
      }

      QMenu contextMnu( this );

      QAction *clearImageAct = new QAction( tr( "Clear Photo" ), this );
      QAction *changeImageAct = new QAction( tr( "Change Photo" ), this );

      connect( clearImageAct , SIGNAL( triggered() ), this, SLOT( clearimage() ) );
      connect( changeImageAct , SIGNAL( triggered() ), this, SLOT( selectimagefile() ) );

      contextMnu.addAction( clearImageAct );
      contextMnu.addAction( changeImageAct );

      contextMnu.exec(QCursor::pos());
}


void ProfileView::profileCustomPopupMenu( QPoint /*point*/ )
{
      if (!mIsOwnId)
      {
      	return;
      }

      QMenu contextMnu( this );

      QAction *editAct = new QAction( tr( "Edit Profile" ), this );
      connect( editAct , SIGNAL( triggered() ), this, SLOT( profileEdit() ) );

      contextMnu.addAction( editAct );

      contextMnu.exec(QCursor::pos());
}

void ProfileView::fileCustomPopupMenu( QPoint /*point*/ )
{
      QMenu contextMnu( this );

      QAction *downloadAct = NULL;
      QAction *downloadAllAct = NULL;
      QAction *removeAct = NULL;
      QAction *clearAct = NULL;

      if (mIsOwnId)
      {
        removeAct = new QAction( tr( "Remove Favourite" ), this );
      	clearAct = new QAction( tr( "Clear Favourites" ), this );
      	connect( removeAct , SIGNAL( triggered() ), this, SLOT( fileRemove() ) );
      	connect( clearAct , SIGNAL( triggered() ), this, SLOT( filesClear() ) );

        contextMnu.addAction( clearAct );
        contextMnu.addAction( removeAct );
      }
      else
      {
      	downloadAct = new QAction( tr( "Download File" ), this );
      	downloadAllAct = new QAction( tr( "Download All" ), this );
      	connect( downloadAct , SIGNAL( triggered() ), this, SLOT( fileDownload() ) );
      	connect( downloadAllAct , SIGNAL( triggered() ), this, SLOT( filesDownloadAll() ) );

        contextMnu.addAction( downloadAct );
        contextMnu.addAction( downloadAllAct );
      }

      contextMnu.exec(QCursor::pos());
}


void ProfileView::setPeerId(std::string id)
{
	pId = id;
	update();
}


void ProfileView::expand()
{
	return;
}


void ProfileView::closeView()
{
	close();
	return;
}


void ProfileView::clear()
{
	return;
}

void ProfileView::update()
{
	/* load it up! */

	/* if id bad -> clear */

	//if (ownId)
	//{
	//	isOwnId = true;
	//}

	mIsOwnId = true; /* switche on context menues */


    uint32_t PostTs;
	std::wstring BlogPost;
	std::list< std::pair<std::wstring, std::wstring> > profile;
	std::list< std::pair<std::wstring, std::wstring> >::iterator pit;
	std::list<FileInfo> files;
	std::list<FileInfo>::iterator fit;
	
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(pId, detail))
	{
            QMessageBox::information(this,
                     tr("RetroShare"),
                     tr("Error : cannot get peer details."));
        }


	ui.idLineEdit->setText(QString::fromStdString(pId));
	ui.nameLineEdit->setText(QString::fromUtf8(detail.name.c_str()));
	{
		std::ostringstream out;
		out << PostTs;
		ui.timeLineEdit->setText(QString::fromStdString(out.str()));
	}
	ui.postTextEdit->setHtml(QString::fromStdWString(BlogPost));

	QList<QTreeWidgetItem *> itemList;
	for(pit = profile.begin(); pit != profile.end(); pit++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(0, QString::fromStdWString(pit->first));
		item->setText(1, QString::fromStdWString(pit->second));

		itemList.push_back(item);
	}

	ui.profileTreeWidget->clear();
	ui.profileTreeWidget->insertTopLevelItems(0, itemList);

	QList<QTreeWidgetItem *> fileList;
	for(fit = files.begin(); fit != files.end(); fit++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(0, QString::fromStdString(fit->fname));
		{
			std::ostringstream out;
			out << fit-> size;
			item->setText(1, QString::fromStdString(out.str()));
		}
		item->setText(2, QString::fromStdString(fit->hash));

		fileList.push_back(item);
	}

	ui.fileTreeWidget->clear();
	ui.fileTreeWidget->insertTopLevelItems(0, fileList);

	return;
}

/* For context Menus */
/* Image Context Menu */
void ProfileView::selectimagefile()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load File",
							QDir::homePath(),
							"Pictures (*.png *.xpm *.jpg)");
	if(!fileName.isEmpty())
	{
		picture = QPixmap(fileName).scaled(108,108, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
		ui.photoLabel->setPixmap(picture);
	}
}

void ProfileView::clearimage()
{
	return;
}


/* for Profile */
void ProfileView::profileEdit()
{
static ProfileEdit *edit = new ProfileEdit(NULL);
	edit->update();
	edit->show();

	return;
}

/* for Favourite Files */
void ProfileView::fileDownload()
{
	return;
}

void ProfileView::filesDownloadAll()
{
	return;
}


void ProfileView::fileRemove()
{
	return;
}

void ProfileView::filesClear()
{
	return;
}

/* add must be done from Shared Files */

void ProfileView::loadAvatar()
{

	QPixmap avatar;
	AvatarDefs::getAvatarFromSslId(pId, avatar, ":/images/user/personal64.png");
	ui.avatarlabel->setPixmap(avatar);
	ui.photoLabel->setPixmap(avatar);
}  

