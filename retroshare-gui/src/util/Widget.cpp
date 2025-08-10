/*******************************************************************************
 * util/Widget.cpp                                                             *
 *                                                                             *
 * Copyright (c) 2006  Crypton                 <retroshare.project@gmail.com>  *
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

#include <util/Widget.h>

#include <QDialog>
#include <QWidget>
#include <QGridLayout>

QGridLayout * Widget::createLayout(QWidget * parent) {
	QGridLayout * layout = new QGridLayout(parent);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	return layout;
}

QDialog * Widget::transformToWindow(QWidget * widget) {
	QDialog * dialog = new QDialog(widget->parentWidget());
	widget->setParent(NULL);
	createLayout(dialog)->addWidget(widget);
	return dialog;
}
