/*******************************************************************************
 * gui/common/RSTextEdit.h                                                     *
 *                                                                             *
 * Copyright (C) 2014 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef RSTEXTEDIT_H
#define RSTEXTEDIT_H

#include <QTextEdit>

class RSTextEdit : public QTextEdit
{
	Q_OBJECT

public:
	RSTextEdit(QWidget *parent = 0);

#if QT_VERSION < 0x050200
	void setPlaceholderText(const QString &text);
#endif

protected:
#if QT_VERSION < 0x050200
	virtual void paintEvent(QPaintEvent *event);
#endif

private:
#if QT_VERSION < 0x050200
	QString mPlaceholderText;
#endif
};

#endif // RSTEXTEDIT_H
