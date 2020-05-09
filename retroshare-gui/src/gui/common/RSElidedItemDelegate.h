/*******************************************************************************
 * gui/common/RsEdlideLabelItemDelegate.h                                      *
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

#ifndef RSELIDEDITEMDELEGATE_H
#define RSELIDEDITEMDELEGATE_H

#include <gui/common/RSItemDelegate.h>

class RSElidedItemDelegate : public RSStyledItemDelegate
{
	Q_OBJECT
	Q_PROPERTY(bool isOnlyPlainText READ isOnlyPlainText WRITE setOnlyPlainText)
	Q_PROPERTY(bool paintRoundedRect READ paintRoundedRect WRITE setPaintRoundedRect)

public:
	RSElidedItemDelegate(QObject *parent = 0);

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

	bool isOnlyPlainText() const { return mOnlyPlainText; }
	void setOnlyPlainText(const bool &value) { mOnlyPlainText = value; }
	bool paintRoundedRect() const { return mPaintRoundedRect; }
	void setPaintRoundedRect(const bool &value) { mPaintRoundedRect = value; }

protected:
	bool editorEvent(QEvent *event,
	                 QAbstractItemModel *model,
	                 const QStyleOptionViewItem &option,
	                 const QModelIndex &index) override;

private:
	bool mOnlyPlainText;
	bool mPaintRoundedRect;

};

#endif // RSELIDEDITEMDELEGATE_H
