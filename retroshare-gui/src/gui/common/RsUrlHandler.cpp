/*******************************************************************************
 * gui/common/RSUrlHandler.h                                                   *
 *                                                                             *
 * Copyright (C) 2011 Cyril Soler     <retroshare.project.com>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <stdexcept>
#include <QDesktopServices>
#include <QUrl>
#include "RsCollection.h"
#include "RsUrlHandler.h"

bool RsUrlHandler::openUrl(const QUrl& url)
{
	if(url.scheme() == QString("file") && url.toLocalFile().endsWith("."+RsCollection::ExtensionString))
	{
		RsCollection collection ;
		if(collection.load(url.toLocalFile()))
		{
			collection.downloadFiles() ;
			return true;
		}
	}
	return QDesktopServices::openUrl(url) ;
}
