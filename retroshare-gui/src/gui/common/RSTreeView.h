/*******************************************************************************
 * gui/common/RSTreeView.h                                                     *
 *                                                                             *
 * Copyright (C) 2010 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef _RSTREEVIEW_H
#define _RSTREEVIEW_H

#include <QTreeView>

/* Subclassing QTreeView */
class RSTreeView : public QTreeView
{
	Q_OBJECT

public:
	RSTreeView(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);

    // Use this to make selection automatic based on mouse position. This is useful to trigger selection and therefore editing mode
    // in trees that show editing widgets using a QStyledItemDelegate

    void setAutoSelect(bool b);

signals:
    void sizeChanged(QSize);

protected:
	virtual void mouseMoveEvent(QMouseEvent *e) override; // overriding so as to manage auto-selection
    virtual void resizeEvent(QResizeEvent *e) override;
	virtual void paintEvent(QPaintEvent *event) override;

	QString placeholderText;
};

#endif
