/*******************************************************************************
 * gui/common/RSUrlHandler.h                                                   *
 *                                                                             *
 * Copyright (C) 2011 Cyril Soler     <retroshare.project@gmail.com>           *
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

// This class overrides the desktop url handling. It's used to have e.g. rsCollection files
// openned by retroshare. Other urls are passed on to the system.
//
class QUrl ;

class RsUrlHandler
{
	public:
		static bool openUrl(const QUrl& url) ;
};

