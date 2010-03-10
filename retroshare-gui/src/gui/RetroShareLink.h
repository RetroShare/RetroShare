/***************************************************************************
 *   Copyright (C) 2009                                                    *
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
#include <stdint.h>
#include <QString>
#include <QVector>

class RetroShareLink
{
	public:
		RetroShareLink(const QString& url);
		RetroShareLink(const QString& name, uint64_t size, const QString& hash);

		uint64_t size() const { return _size ; }
		const QString& name() const { return _name ; }
		const QString& hash() const { return _hash ; }

		QString toString() const ;

		bool valid() const { return _size > 0 ; }

		static void parseForLinks(const QString& text,QList<RetroShareLink>& link_list) ;
	private:
		void check() ;
		static bool checkHash(const QString& hash) ;
		static bool checkName(const QString& hash) ;

		static const QString HEADER_NAME;

		QString 	_name;
		uint64_t _size;
		QString 	_hash;
};

#endif

