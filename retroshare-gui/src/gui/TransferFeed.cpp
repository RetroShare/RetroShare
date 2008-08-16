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
#include <QtGui>

#include <iostream>

#include "TransferFeed.h"
#include "feeds/SubFileItem.h"
#include "GeneralMsgDialog.h"

#include "rsiface/rsfiles.h"


/*****
 * #define TRANSFER_DEBUG  1
 ****/

/** Constructor */
TransferFeed::TransferFeed(QWidget *parent)
: MainPage (parent)
{
  	/* Invoke the Qt Designer generated object setup routine */
  	setupUi(this);

  	connect( modeComboBox, SIGNAL( currentIndexChanged( int ) ), this, SLOT( updateMode() ) );

  {
	/* mLayout -> to add widgets to */
	mDownloadsLayout = new QVBoxLayout;

	QWidget *middleWidget = new QWidget();
	//middleWidget->setSizePolicy( QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(mDownloadsLayout);


     	QScrollArea *scrollArea = new QScrollArea;
        //scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	layout2->setSpacing(0);
	layout2->setMargin(0);
	
     	frameDown->setLayout(layout2);
  }

  {
	/* mLayout -> to add widgets to */
	mUploadsLayout = new QVBoxLayout;

	QWidget *middleWidget = new QWidget();
	//middleWidget->setSizePolicy( QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(mUploadsLayout);


     	QScrollArea *scrollArea = new QScrollArea;
        //scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	layout2->setSpacing(0);
	layout2->setMargin(0);
	
     	frameUp->setLayout(layout2);
  }

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateAll()));
	timer->start(1000);

}



/* FeedHolder Functions (for FeedItem functionality) */
void TransferFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
#ifdef TRANSFER_DEBUG  
	std::cerr << "TransferFeed::deleteFeedItem()";
	std::cerr << std::endl;
#endif
}

void TransferFeed::openChat(std::string peerId)
{
#ifdef TRANSFER_DEBUG  
	std::cerr << "TransferFeed::openChat()";
	std::cerr << std::endl;
#endif
}

void TransferFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
#ifdef TRANSFER_DEBUG  
	std::cerr << "TransferFeed::openMsg()";
	std::cerr << std::endl;
#endif

	GeneralMsgDialog *msgDialog = new GeneralMsgDialog(NULL);


	msgDialog->addDestination(type, grpId, inReplyTo);

	msgDialog->show();

}

void  TransferFeed::updateMode()
{
	updateAll();
}

void  TransferFeed::updateAll()
{
	updateDownloads();
	updateUploads();
}

void  TransferFeed::updateDownloads()
{
	std::list<std::string> hashes, toAdd, toRemove;
	std::list<std::string>::iterator it;

	std::map<std::string, uint32_t> newhashes;
	std::map<std::string, uint32_t>::iterator nit;
	std::map<std::string, SubFileItem *>::iterator fit;

	if (!rsFiles)
        {
                /* not ready yet! */
                return;
        }

	rsFiles->FileDownloads(hashes);

	for(it = hashes.begin(); it != hashes.end(); it++)
	{
		newhashes[*it] = 1;
	}

	nit = newhashes.begin();
	fit = mDownloads.begin();

	while(nit != newhashes.end())
	{
		if (fit == mDownloads.end())
		{
			toAdd.push_back(nit->first);
			nit++;
			continue;
		}

		if (nit->first == fit->first)
		{
			/* same - good! */
			nit++;
			fit++;
			continue;
		}

		if (nit->first < fit->first)
		{
			/* must add in new item */
			toAdd.push_back(nit->first);
			nit++;
		}
		else
		{
			toRemove.push_back(fit->first);
			fit++;
		}
	}

	/* remove remaining items */
	while (fit != mDownloads.end())
	{
		toRemove.push_back(fit->first);
		fit++;
	}

	/* remove first */
	for(it = toRemove.begin(); it != toRemove.end(); it++)
	{
		fit = mDownloads.find(*it);
		if (fit != mDownloads.end())
		{
			delete (fit->second);
			mDownloads.erase(fit);
		}
	}

	/* add in new ones */
	for(it = toAdd.begin(); it != toAdd.end(); it++)
	{
		FileInfo fi;
		uint32_t hintflags = RS_FILE_HINTS_DOWNLOAD 
					| RS_FILE_HINTS_SPEC_ONLY;

		if (rsFiles->FileDetails(*it, hintflags, fi))
		{
  			SubFileItem *sfi = new SubFileItem(*it, fi.fname, 
				fi.size, SFI_STATE_DOWNLOAD, fi.source);
			mDownloads[*it] = sfi;
			mDownloadsLayout->addWidget(sfi);
		}
	}
}

void  TransferFeed::updateUploads()
{
	std::list<std::string> hashes, toAdd, toRemove;
	std::list<std::string>::iterator it;

	std::map<std::string, uint32_t> newhashes;
	std::map<std::string, uint32_t>::iterator nit;
	std::map<std::string, SubFileItem *>::iterator fit;

	if (!rsFiles)
        {
                /* not ready yet! */
                return;
        }

	rsFiles->FileUploads(hashes);

	for(it = hashes.begin(); it != hashes.end(); it++)
	{
		newhashes[*it] = 1;
	}

	nit = newhashes.begin();
	fit = mUploads.begin();

	while(nit != newhashes.end())
	{
		if (fit == mUploads.end())
		{
			toAdd.push_back(nit->first);
			nit++;
			continue;
		}

		if (nit->first == fit->first)
		{
			/* same - good! */
			nit++;
			fit++;
			continue;
		}

		if (nit->first < fit->first)
		{
			/* must add in new item */
			toAdd.push_back(nit->first);
			nit++;
		}
		else
		{
			toRemove.push_back(fit->first);
			fit++;
		}
	}

	/* remove remaining items */
	while (fit != mUploads.end())
	{
		toRemove.push_back(fit->first);
		fit++;
	}

	/* remove first */
	for(it = toRemove.begin(); it != toRemove.end(); it++)
	{
		fit = mUploads.find(*it);
		if (fit != mUploads.end())
		{
			delete (fit->second);
			mUploads.erase(fit);
		}
	}

	/* add in new ones */
	for(it = toAdd.begin(); it != toAdd.end(); it++)
	{
		FileInfo fi;
		uint32_t hintflags = RS_FILE_HINTS_UPLOAD 
					| RS_FILE_HINTS_SPEC_ONLY;

		if (rsFiles->FileDetails(*it, hintflags, fi))
		{
  			SubFileItem *sfi = new SubFileItem(*it, fi.fname, 
				fi.size, SFI_STATE_UPLOAD, fi.source);
			mUploads[*it] = sfi;
			mUploadsLayout->addWidget(sfi);
		}
	}
}



