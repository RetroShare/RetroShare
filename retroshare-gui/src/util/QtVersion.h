/*******************************************************************************
 * util/qthreadutils.h                                                         *
 *                                                                             *
 * Copyright (C) 2013 Retroshare Team <retroshare.project@gmail.com>           *
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

#ifndef QTVERSION_H
#define QTVERSION_H

// Macros to compile with Qt 4 and Qt 5

// Renamed QHeaderView::setResizeMode to QHeaderView::setSectionResizeMode
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#define QHeaderView_setSectionResizeModeColumn(header, column, mode) header->setSectionResizeMode(column, mode);
#else
#define QHeaderView_setSectionResizeModeColumn(header, column, mode) header->setResizeMode(column, mode);
#endif

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#define QHeaderView_setSectionResizeMode(header, mode) header->setSectionResizeMode(mode);
#else
#define QHeaderView_setSectionResizeMode(header, mode) header->setResizeMode(mode);
#endif

// Renamed QHeaderView::setMovable to QHeaderView::setSectionsMovable
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
#define QHeaderView_setSectionsMovable(header, movable) header->setSectionsMovable(movable);
#else
#define QHeaderView_setSectionsMovable(header, movable) header->setMovable(movable);
#endif

#endif
