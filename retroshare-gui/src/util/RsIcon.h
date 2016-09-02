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

#ifndef RSICON_H
#define RSICON_H

#include <QIcon>

#include "util/RsIconEngine.h"

class RsIcon : public QIcon
{
public:
	explicit RsIcon();
	explicit RsIcon(const QString &fileName, const bool onNotify = false); // file or resource name
	~RsIcon();

	bool onNotify() const;
	void setOnNotify(const bool value);

private:
	RsIconEngine *m_Engine;
};

#endif // RSICON_H
