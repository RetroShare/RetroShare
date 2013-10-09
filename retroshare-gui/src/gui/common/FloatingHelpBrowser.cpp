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

#include <QGraphicsDropShadowEffect>
#include <QAbstractButton>
#include <QShowEvent>

#include <iostream>

#include "FloatingHelpBrowser.h"

FloatingHelpBrowser::FloatingHelpBrowser(QWidget *parent, QAbstractButton *button) :
	QTextBrowser(parent), mButton(button)
{
	QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect(this);
	effect->setBlurRadius(30.0);
	setGraphicsEffect(effect);

	setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum));
	hide();

	connect(this, SIGNAL(textChanged()), this, SLOT(textChanged()));

	if (mButton) {
		connect(mButton, SIGNAL(toggled(bool)), this, SLOT(showHelp(bool)));
	}
}

void FloatingHelpBrowser::showEvent(QShowEvent */*event*/)
{
	if (mButton) {
		mButton->setChecked(true);
	}
}

void FloatingHelpBrowser::hideEvent(QHideEvent */*event*/)
{
	if (mButton) {
		mButton->setChecked(false);
	}
}

void FloatingHelpBrowser::mousePressEvent(QMouseEvent */*event*/)
{
	hide();
}

void FloatingHelpBrowser::textChanged()
{
	if (mButton) {
		mButton->setVisible(!document()->isEmpty());
	}
}

void FloatingHelpBrowser::showHelp(bool state)
{
	QWidget *p = parentWidget();
	if (!p) {
		return;
	}

	resize(p->size() * 0.5);
	move(p->width() / 2 - width() / 2, p->height() / 2 - height() / 2);
	update();

	std::cerr << "Toggling help to " << state << std::endl;

	setVisible(state);
}

void FloatingHelpBrowser::setHelpText(const QString &helpText)
{
	if (helpText.isEmpty()) {
		clear();
		return;
	}

	setHtml("<div align=\"justify\">" + helpText + "</div>");
}
