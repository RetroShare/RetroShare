/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, Cyril Soler (csoler@users.sourceforge.net)
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

#include <stdexcept>
#include <QDesktopServices>
#include <QMessageBox>
#include <QUrl>
#include "RsCollectionFile.h"
#include "RsUrlHandler.h"

bool RsUrlHandler::openUrl(const QUrl& url)
{
	if(url.scheme() == QString("file") && url.toLocalFile().endsWith(RsCollectionFile::ExtensionString))
	{
		try
		{
			RsCollectionFile(url.toLocalFile().toUtf8().constData()).downloadFiles() ;
		}
		catch(std::runtime_error& e)
		{
			QMessageBox::warning(NULL,QObject::tr("Treatment of collection file has failed."),QObject::tr("The collection file ") + url.toLocalFile() + QObject::tr(" could not be openned. Reported error is: ") + QString::fromStdString(e.what())) ;
			return false ;
		}
	}
	else
		return QDesktopServices::openUrl(url) ;
}



