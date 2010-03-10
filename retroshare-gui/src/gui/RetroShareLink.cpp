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

#include <iostream>
#include <QStringList>
#include "RetroShareLink.h"

#define DEBUG_RSLINK 1

const QString RetroShareLink::HEADER_NAME("retroshare://file");

RetroShareLink::RetroShareLink(const QUrl& url)
{
	// parse
#ifdef DEBUG_RSLINK
	std::cerr << "got new RS link \"" << url.toString().toStdString() << "\"" << std::endl ;
#endif
	QStringList list = url.toString().split ("|");

	if(list.size() < 4)
		goto bad_link ;

	bool ok ;

	_name = list[1] ;
	_size = list[2].toULongLong(&ok) ;
	_hash = list[3].left(40) ;	// normally not necessary, but it's a security.

	if(!ok)
		goto bad_link ;
#ifdef DEBUG_RSLINK
	std::cerr << "New RetroShareLink forged:" << std::endl ;
	std::cerr << "  name = \"" << _name.toStdString() << "\"" << std::endl ;
	std::cerr << "  hash = \"" << _hash.toStdString() << "\"" << std::endl ;
	std::cerr << "  size = " << _size << std::endl ;
#endif

	 check() ;

	 return ;
bad_link:
#ifdef DEBUG_RSLINK
	 std::cerr << "Wrongly formed RS link. Can't process." << std::endl ;
#endif
	 _hash = "" ;
	 _size = 0 ;
	 _name = "" ;
}

RetroShareLink::RetroShareLink(const QString & name, uint64_t size, const QString & hash)
	: _name(name),_size(size),_hash(hash)
{
	check() ;
}

void RetroShareLink::check()
{
	bool valid = true ;

	if(_size > (((uint64_t)1)<<40))	// 1TB. Who has such large files?
		valid = false ;

	if(!checkName(_name))
		valid = false ;

	if(!checkHash(_hash))
		valid = false ;

	if(!valid) // we should throw an exception instead of this crap, but drbob doesn't like exceptions. Why ???
	{
		_hash = "" ;
		_name = "" ;
		_size = 0 ;
	}
}

QString RetroShareLink::toString() const
{
	return HEADER_NAME + "|" + _name + "|" + QString::number(_size) + "|" + _hash ;
}

bool RetroShareLink::checkName(const QString& name)
{
	if(name == "") 
		return false ;

	for(int i=0;i<name.length();++i)
	{
		QChar::Category cat( name[i].category() ) ;

		if(	cat == QChar::Separator_Line
			|| cat == QChar::Other_NotAssigned
			)
		{
#ifdef DEBUG_RSLINK
			std::cerr <<"Unwanted category " << cat << " at place " << i << " in string \"" << name.toStdString() << "\"" << std::endl ;
#endif
			return false ;
		}
	}

	return true ;
}

QUrl RetroShareLink::toUrl() const
{
	return QUrl(toString()) ;
}

bool RetroShareLink::checkHash(const QString& hash)
{
	if(hash.length() != 40)
		return false ;

	QByteArray qb(hash.toAscii()) ;

	for(int i=0;i<qb.length();++i)
	{
		unsigned char b(qb[i]) ;

		if(!((b>47 && b<58) || (b>96 && b<103)))
			return false ;
	}

	return true ;
}

