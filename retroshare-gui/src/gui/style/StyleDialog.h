/*******************************************************************************
 * gui/style/StyleDialog.h                                                     *
 *                                                                             *
 * Copyright (c) 2006 Crypton         <retroshare.project@gmail.com>           *
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

#ifndef _STYLEDIALOG_H
#define _STYLEDIALOG_H

#include <QMap>
#include <QList>

#include "ui_StyleDialog.h"

class QPushButton;
class RSStyle;

class StyleDialog : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
	StyleDialog(RSStyle &style, QWidget *parent = 0);
	/** Default destructor */
	~StyleDialog();

	void getStyle(RSStyle &style);

private slots:
	void chooseColor();
	void showButtons();

private:
	QMap<QPushButton*, QColor> colors;
	QList<QPushButton*> pushButtons;
	QList<QLabel*> labels;

	void drawButtons();
	int neededColors();
	void drawPreview();

	/** Qt Designer generated object */
	Ui::StyleDialog ui;
};

#endif
