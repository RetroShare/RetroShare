/*******************************************************************************
 * gui/common/RSPlainTextEdit.h                                                *
 *                                                                             *
 * Copyright (C) 2013 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef RSPLAINTEXTEDIT_H
#define RSPLAINTEXTEDIT_H

#include <QPlainTextEdit>

class RSPlainTextEdit : public QPlainTextEdit
{
	Q_OBJECT

public:
	RSPlainTextEdit(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);

protected:
	void paintEvent(QPaintEvent *event);

private:
	QString mPlaceholderText;
};

#endif // RSPLAINTEXTEDIT_H
