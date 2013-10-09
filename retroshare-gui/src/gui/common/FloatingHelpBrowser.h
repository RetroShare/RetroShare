/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2013, RetroShare Team
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
 
#ifndef FLOATINGHELPBROWSER_H
#define FLOATINGHELPBROWSER_H

#include <QTextBrowser>

class QAbstractButton;

class FloatingHelpBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	explicit FloatingHelpBrowser(QWidget *parent, QAbstractButton *button);

	void setHelpText(const QString &helpText);

public slots:
	void showHelp(bool state);

private slots:
	void textChanged();

protected:
	virtual void showEvent(QShowEvent *event);
	virtual void hideEvent(QHideEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);

protected:
	using QTextBrowser::setHtml;
	using QTextBrowser::setText;

private:
	QAbstractButton *mButton;
};

#endif // FLOATINGHELPBROWSER_H
