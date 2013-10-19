/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 RetroShare Team
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

#ifndef QTVERSION_H
#define QTVERSION_H

// Macros to compile with Qt 4 and Qt 5

// Renamed QHeaderView::setResizeMode to QHeaderView::setSectionResizeMode
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#define QHeaderView_setSectionResizeMode(header, column, mode) header->setSectionResizeMode(column, mode);
#else
#define QHeaderView_setSectionResizeMode(header, column, mode) header->setResizeMode(column, mode);
#endif

// Renamed QHeaderView::setMovable to QHeaderView::setSectionsMovable
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#define QHeaderView_setSectionsMovable(header, movable) header->setSectionsMovable(movable);
#else
#define QHeaderView_setSectionsMovable(header, movable) header->setMovable(movable);
#endif

#endif
