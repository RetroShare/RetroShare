/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include <QFile>
#include <QFileInfo>

#include "rshare.h"
#include "common/vmessagebox.h"
#include "NetworkDialog.h"
#include "connect/ConnectDialog.h"
#include "authdlg/AuthorizationDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rstypes.h"
#include <sstream>


#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

/* Images for context menu icons */
#define IMAGE_LOADCERT       ":/images/loadcert16.png"
#define IMAGE_PEERDETAILS    ":/images/peerdetails_16x16.png"
#define IMAGE_AUTH           ":/images/encrypted16.png"
/* Images for Status icons */
#define IMAGE_AUTHED         ":/images/dauthed.png"
#define IMAGE_DENIED         ":/images/ddeny.png"

RsCertId getNeighRsCertId(QTreeWidgetItem *i);

/** Constructor */
NetworkDialog::NetworkDialog(QWidget *parent)
: MainPage(parent), connectdialog(NULL)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.connecttreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( connecttreeWidgetCostumPopupMenu( QPoint ) ) );

  /* create a single connect dialog */
  connectdialog = new ConnectDialog();
  
  /* hide the Tree +/- */
  ui.connecttreeWidget -> setRootIsDecorated( false );
  
  /* Set header resize modes and initial section sizes */
	QHeaderView * _header = ui.connecttreeWidget->header () ;   
	_header->setResizeMode (0, QHeaderView::Custom);
	_header->setResizeMode (1, QHeaderView::Interactive);
	_header->setResizeMode (2, QHeaderView::Interactive);
	_header->setResizeMode (3, QHeaderView::Interactive);
	_header->setResizeMode (4, QHeaderView::Interactive);
	_header->setResizeMode (5, QHeaderView::Interactive);
	_header->setResizeMode (6, QHeaderView::Interactive);
	_header->setResizeMode (7, QHeaderView::Interactive);
	_header->setResizeMode (8, QHeaderView::Interactive);
	_header->setResizeMode (9, QHeaderView::Interactive);
	_header->setResizeMode (10, QHeaderView::Interactive);
    
	_header->resizeSection ( 0, 25 );
	_header->resizeSection ( 1, 100 );
	_header->resizeSection ( 2, 100 );
	_header->resizeSection ( 3, 100 );
	_header->resizeSection ( 4, 100 );
	_header->resizeSection ( 5, 200);
	_header->resizeSection ( 6, 100 );
	_header->resizeSection ( 7, 100 );
	_header->resizeSection ( 8, 100 );
	_header->resizeSection ( 9, 100 );
     

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void NetworkDialog::connecttreeWidgetCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      peerdetailsAct = new QAction(QIcon(IMAGE_PEERDETAILS), tr( "Peer Details / Authenticate " ), this );
      connect( peerdetailsAct , SIGNAL( triggered() ), this, SLOT( peerdetails() ) );
      
      //authAct = new QAction(QIcon(IMAGE_AUTH), tr( "Authenticate" ), this );
      //connect( authAct , SIGNAL( triggered() ), this, SLOT( peerdetails() ) );
      
      loadcertAct = new QAction(QIcon(IMAGE_LOADCERT), tr( "Load Certificate" ), this );
      connect( loadcertAct , SIGNAL( triggered() ), this, SLOT( loadneighbour() ) );


      contextMnu.clear();
      contextMnu.addAction( peerdetailsAct);
      //contextMnu.addAction( authAct);
      contextMnu.addAction( loadcertAct);
      contextMnu.exec( mevent->globalPos() );
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::peerdetails()
{
    std::cerr << "ConnectionsDialog::peerdetails()" << std::endl;

    QTreeWidgetItem *wi = getCurrentNeighbour();
    if (!wi)
    	return;

    RsCertId id = getNeighRsCertId(wi);
    std::ostringstream out;
    out << id;

    showpeerdetails(out.str());
}

/** Shows Peer Information/Auth Dialog */
void NetworkDialog::showpeerdetails(std::string id)
{
    std::cerr << "ConnectionsDialog::showpeerdetails()" << std::endl;
    if ((connectdialog) && (connectdialog -> loadPeer(id)))
    {
    	connectdialog->show();
    }
}

/** Shows Connect Dialog */
void NetworkDialog::showAuthDialog()
{
    static AuthorizationDialog *authorizationdialog = new AuthorizationDialog();
    QTreeWidgetItem *wi = getCurrentNeighbour();
    if (!wi)
    	return;

    RsCertId id = getNeighRsCertId(wi);
    std::ostringstream out;
    out << id;
    authorizationdialog->setAuthCode(out.str(), wi->text(9).toStdString());
    authorizationdialog->show();
}




/** Open a QFileDialog to browse for a pem/pqi file. */
void NetworkDialog::loadcert()
{
  /* Create a new input dialog, which allows users to create files, too */
  QFileDialog *dialog = new QFileDialog(this, tr("Select a pem/pqi File"));
  //dialog->setDirectory(QFileInfo(ui.lineTorConfig->text()).absoluteDir());
  //dialog->selectFile(QFileInfo(ui.lineTorConfig->text()).fileName());
  dialog->setFileMode(QFileDialog::AnyFile);
  dialog->setReadOnly(false);

  /* Prompt the user to select a file or create a new one */
  if (!dialog->exec() || dialog->selectedFiles().isEmpty()) {
    return;
  }
  QString filename = QDir::convertSeparators(dialog->selectedFiles().at(0));
 
  /* Check if the file exists */
  QFile torrcFile(filename);
  if (!QFileInfo(filename).exists()) {
    /* The given file does not exist. Should we create it? */
    int response = VMessageBox::question(this,
                     tr("File Not Found"),
                     tr("%1 does not exist. Would you like to create it?")
                                                            .arg(filename),
                     VMessageBox::Yes, VMessageBox::No);
    
    if (response == VMessageBox::No) {
      /* Don't create it. Just bail. */
      return;
    }
    /* Attempt to create the specified file */
    if (!torrcFile.open(QIODevice::WriteOnly)) {
      VMessageBox::warning(this,
        tr("Failed to Create File"),
        tr("Unable to create %1 [%2]").arg(filename)
                                      .arg(torrcFile.errorString()),
        VMessageBox::Ok);
      return;
    }
  }
  //ui.lineTorConfig->setText(filename);
}



#include <sstream>

/* get the list of Neighbours from the RsIface.  */
void NetworkDialog::insertConnect()
{
        rsiface->lockData(); /* Lock Interface */

        std::map<RsCertId,NeighbourInfo>::const_iterator it;
        const std::map<RsCertId,NeighbourInfo> &neighs = 
				rsiface->getNeighbourMap();

	/* get a link to the table */
        QTreeWidget *connectWidget = ui.connecttreeWidget;
	QTreeWidgetItem *oldSelect = getCurrentNeighbour();
	QTreeWidgetItem *newSelect = NULL;
	std::string oldId;
	if (oldSelect)
	{
		oldId = (oldSelect -> text(9)).toStdString();
	}

	/* remove old items ??? */
	connectWidget->clear();
	connectWidget->setColumnCount(11);	

        QList<QTreeWidgetItem *> items;
	for(it = neighs.begin(); it != neighs.end(); it++)
	{
		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		
	    /* (0) Status Icon */
		        item -> setText(0, "");

		/* (1) Accept/Deny */
                item -> setText(1, QString::fromStdString(it->second.acceptString));
		/* (2) Trust Level */
                item -> setText(2, QString::fromStdString(it->second.trustString));
		/* (3) Last Connect */
                item -> setText(3, QString::fromStdString(it->second.lastConnect));
        /* (4) Person */
				item -> setText(4, QString::fromStdString(it->second.name));
		
		/* (5) Peer Address */
				item -> setText(5, QString::fromStdString(it->second.peerAddress));

		/* Others */
				item -> setText(6, QString::fromStdString(it->second.org));
				item -> setText(7, QString::fromStdString(it->second.loc));
				item -> setText(8, QString::fromStdString(it->second.country));


		{
			std::ostringstream out;
			out << it -> second.id;
			item -> setText(9, QString::fromStdString(out.str()));
			if ((oldSelect) && (oldId == out.str()))
			{
				newSelect = item;
			}
		}

			item -> setText(10, QString::fromStdString(it->second.authCode));


		/* change background */
		int i;
                if (it->second.acceptString == "Accept")
		{
                	if (it->second.lastConnect != "Never")
			{
				/* bright green */
				for(i = 1; i < 11; i++)
				{
				  item -> setBackground(i,QBrush(Qt::darkGreen));
				  item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
				}
			}
			else
			{
				for(i = 1; i < 11; i++)
				{
				  item -> setBackground(i,QBrush(Qt::darkGreen));
				  item -> setIcon(0,(QIcon(IMAGE_AUTHED)));
				}
			}
		}
		else
		{
                	if (it->second.trustLvl > 3)
			{
				for(i = 1; i < 11; i++)
				{
				  item -> setBackground(i,QBrush(Qt::cyan));
				  item -> setIcon(0,(QIcon(IMAGE_DENIED)));
				}
			}
                	else if (it->second.lastConnect != "Never")
			{
				for(i = 1; i < 11; i++)
				{
				  item -> setBackground(i,QBrush(Qt::yellow));
				  item -> setIcon(0,(QIcon(IMAGE_DENIED)));
				}
			}
			else
			{
				for(i = 1; i < 11; i++)
				{
				  item -> setBackground(i,QBrush(Qt::gray));
				  item -> setIcon(0,(QIcon(IMAGE_DENIED)));
				}
			}
		}
			

		/* add to the list */
		items.append(item);
	}

	/* add the items in! */
	connectWidget->insertTopLevelItems(0, items);
	if (newSelect)
	{
		connectWidget->setCurrentItem(newSelect);
	}

	rsiface->unlockData(); /* UnLock Interface */

	connectWidget->update(); /* update display */
}

QTreeWidgetItem *NetworkDialog::getCurrentNeighbour()
{ 
        /* get the current, and extract the Id */
  
        /* get a link to the table */
        QTreeWidget *connectWidget = ui.connecttreeWidget;
        QTreeWidgetItem *item = connectWidget -> currentItem();
        if (!item) 
        {
                std::cerr << "Invalid Current Item" << std::endl;
                return NULL;
        }
    
        /* Display the columns of this item. */

/**** NO NEED ANYMORE
        std::ostringstream out;
        out << "CurrentNeighbourItem: " << std::endl;

        for(int i = 1; i < 6; i++)
        {
                QString txt = item -> text(i);
                out << "\t" << i << ":" << txt.toStdString() << std::endl;
        }
        std::cerr << out.str();
*****************/

        return item;
}   

/* Utility Fns */
RsCertId getNeighRsCertId(QTreeWidgetItem *i)
{
        RsCertId id = (i -> text(9)).toStdString();
        return id;
}   
  
/* So from the Neighbours Dialog we can call the following control Functions:
 * (1) Load Certificate             NeighLoadCertificate(std::string file)
 * (2) Neigh  Auth                  NeighAuthFriend(id, code)
 * (3) Neigh  Add                   NeighAddFriend(id)
 *
 * All of these rely on the finding of the current Id.
 */

std::string NetworkDialog::loadneighbour()
{
        std::cerr << "ConnectionsDialog::loadneighbour()" << std::endl;
        QString fileName = QFileDialog::getOpenFileName(this, tr("Select Certificate"), "",
	                                             tr("Certificates (*.pqi *.pem)"));

	std::string file = fileName.toStdString();
	std::string id;
	if (file != "")
	{
        	rsicontrol->NeighLoadCertificate(file, id);
	}
	return id;
}

void NetworkDialog::addneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
        std::cerr << "ConnectionsDialog::addneighbour()" << std::endl;
        /*
        rsServer->NeighAddFriend(getNeighRsCertId(c));
        */
}

void NetworkDialog::authneighbour()
{
        QTreeWidgetItem *c = getCurrentNeighbour();
        std::cerr << "ConnectionsDialog::authneighbour()" << std::endl;
        /*
	RsAuthId code;
        rsServer->NeighAuthFriend(getNeighRsCertId(c), code);
        */
}

