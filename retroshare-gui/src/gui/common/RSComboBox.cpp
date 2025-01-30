/*******************************************************************************
 * gui/common/RSComboBox.cpp                                                   *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#include "RSComboBox.h"

#include "gui/common/RSElidedItemDelegate.h"

#include <QAbstractItemView>
#include <QEvent>

RSComboBox::RSComboBox(QWidget *parent /*= nullptr*/)
    :QComboBox(parent)
{
	// To Fix ComboBox item delegate not respecting stylesheet. See QDarkStyleSheet issues 214
	setItemDelegate(new RSElidedItemDelegate());
	view()->installEventFilter(this);
}

RSComboBox::~RSComboBox()
{
	delete this->itemDelegate();
}

bool RSComboBox::eventFilter(QObject *obj, QEvent *event)
{
	if(QAbstractItemView* view = dynamic_cast<QAbstractItemView*>(obj))
	{
		// To Fix ComboBox item delegate not respecting stylesheet. See QDarkStyleSheet issues 214
		// With QStyleItemDelegate QComboBox::item:checked doesn't works.
		// With QComboBox ::item:checked, it's applied to hover ones.
		if (event->type() == QEvent::Show)
		{
			if(QComboBox* cmb = dynamic_cast<QComboBox*>(view->parent()->parent()) )
			{
				for (int curs = 0; curs < cmb->count(); ++curs)
				{
					QFont font = cmb->itemData(curs, Qt::FontRole).value<QFont>();
					font.setBold(curs == cmb->currentIndex());
					cmb->setItemData(curs, QVariant(font), Qt::FontRole);

				}
			}
		}
	}
	// pass the event on to the parent class
	return QWidget::eventFilter(obj, event);
}
