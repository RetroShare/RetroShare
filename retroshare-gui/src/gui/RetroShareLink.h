/***************************************************************************
 *   Copyright (C) 2009 Cyril Soler                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef RETROSHARE_LINK_ANALYZER
#define RETROSHARE_LINK_ANALYZER

// This class implements a RS link.
//
// The link is done in a way that if valid()==true, then the link can be used.
//
// The following combinations have been tested:
//  copy   paste->   Transfers (DL) | Message compose (forums) | Private msg      | Public chat | private Chat
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//          search |      Y         |            Y             | Y (send RS link) | Paste menu? | Paste menu?
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//          shared |      Y         |            Y             | Y (send RS link) | Paste menu? | Paste menu?
//    -------------+----------------+--------------------------+------------------+-------------+-------------
//
#include <stdint.h>
#include <QString>
#include <QVector>
#include <QUrl>

class RetroShareLink
{
	public:
		RetroShareLink(const QUrl& url);
		RetroShareLink(const QString& name, uint64_t size, const QString& hash);

		uint64_t size() const { return _size ; }
		const QString& name() const { return _name ; }
		const QString& hash() const { return _hash ; }

		/// returns the string retroshare://file|name|size|hash
		QString toString() const ;
		/// returns the string <a href="retroshare://file|name|size|hash">name</a>
		QString toHtml() const ;
		/// returns the string <a href="retroshare://file|name|size|hash">retroshare://file|name|size|hash</a>
		QString toHtmlFull() const ;

		QUrl toUrl() const ;

		bool valid() const { return _size > 0 ; }

		bool operator==(const RetroShareLink& l) const { return _hash == l._hash ; }
	private:
		void check() ;
		static bool checkHash(const QString& hash) ;
		static bool checkName(const QString& hash) ;

		static const QString HEADER_NAME;

		QString 	_name;
		uint64_t _size;
		QString 	_hash;
};

/// This class handles the copy/paste of links. Every member is static to ensure unicity.
/// I put no mutex, because all calls supposely com from the GUI thread.
/// 
/// All links are stored in html format into the clipboard. Why? Because this allows to import
/// links from both the clipboard and the RS application, e.g. paste links from an internet forum.
/// This requires many clipboard parsing operations, but this is not a problem because this code is
/// not performances-critical.
//
class RSLinkClipboard
{
	public:
		// Copy these links to the RS clipboard. Also copy them to the system clipboard
		//
		static void copyLinks(const std::vector<RetroShareLink>& links) ;

		// Get the liste of pasted links, either from the internal RS links, or by default
		// from the clipboard.
		//
		static std::vector<RetroShareLink> pasteLinks() ;

		// Produces a list of links with no html structure.
		static QString toString() ;

		// produces a list of html links that displays with the file names only
		//
		static QString toHtml();

		// produces a list of html links that displays the full links
		//
		static QString toHtmlFull();

		// Returns true is no links are found to paste.
		// Useful for menus.
		//
		static bool empty();

	private:
		static std::vector<RetroShareLink> parseClipboard() ;
};



#endif

