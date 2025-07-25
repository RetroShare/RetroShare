/*******************************************************************************
 * util/RsQtVersion.h                                                         *
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

#ifndef RS_QTVERSION_H
#define RS_QTVERSION_H

// Macros to compile with Qt 4, Qt 5 and Qt 6

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

#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
#define QtSkipEmptyParts Qt::SkipEmptyParts
#else
#define QtSkipEmptyParts QString::SkipEmptyParts
#endif

#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
#define QSortFilterProxyModel_setFilterRegularExpression(proxyModel, pattern) proxyModel->setFilterRegularExpression(pattern);
#else
#define QSortFilterProxyModel_setFilterRegularExpression(proxyModel, pattern) proxyModel->setFilterRegExp(pattern);
#endif

#if QT_VERSION >= QT_VERSION_CHECK (6, 0, 0)
#define QFontMetrics_horizontalAdvance(fontMetrics, text) fontMetrics.horizontalAdvance(text)
#else
#define QFontMetrics_horizontalAdvance(fontMetrics, text) fontMetrics.width(text)
#endif

#if QT_VERSION >= QT_VERSION_CHECK (6, 6, 0)
#define QLabel_pixmap(label) label->pixmap()
#elif QT_VERSION >= QT_VERSION_CHECK (5, 15, 0)
#define QLabel_pixmap(label) label->pixmap(Qt::ReturnByValue)
#else
class QLabel;
extern QPixmap QLabel_pixmap(QLabel* label);
#endif

#if QT_VERSION < QT_VERSION_CHECK (5, 8, 0)
#define Q_FALLTHROUGH() (void)0
#endif

#endif
