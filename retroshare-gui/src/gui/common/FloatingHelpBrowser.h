/*******************************************************************************
 * gui/common/FloatingHelpBrowser.h                                            *
 *                                                                             *
 * Copyright (C) 2013, Retroshare Team <retroshare.project@gmail.com>          *
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
	void showHelp(bool state=false);

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
