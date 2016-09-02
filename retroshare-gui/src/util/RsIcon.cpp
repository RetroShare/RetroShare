/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2016, RetroShare Team
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
//Inspirated from this code:http://stackoverflow.com/questions/27541821/custom-qicon-using-qiconengine-and-transparency
//But doesn't work. So use Qt source file

#include "RsIcon.h"

RsIcon::RsIcon()
  : QIcon()
{
	this->~QIcon();
	m_Engine = new RsIconEngine();
	new (this) QIcon(m_Engine);
}

RsIcon::RsIcon(const QString &fileName, const bool onNotify /*= false*/)
  : QIcon(fileName)
{
	this->~QIcon();
	m_Engine = new RsIconEngine();
	m_Engine->setOnNotify(onNotify);
	m_Engine->addFile(fileName,QSize(),QIcon::Normal, QIcon::Off);

	if (!m_Engine->isNull()) {
		new (this) QIcon(m_Engine);
	} else {
		new (this) QIcon(fileName);
	}
}

RsIcon::~RsIcon()
{
	//delete m_Engine;//Deleted by QIcon destructor
}

bool RsIcon::onNotify() const
{
	if (m_Engine)
		return m_Engine->onNotify();

	return false;
}

void RsIcon::setOnNotify(const bool value)
{
	if (m_Engine)
		m_Engine->setOnNotify(value);
}
