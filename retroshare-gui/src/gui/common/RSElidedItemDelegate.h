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

#include <QSvgRenderer>

class RSElidedItemDelegate : public RSStyledItemDelegate
{
	Q_OBJECT

	// For now, these properties cannot be changed by StyleSheet
	// If needed, you can add properties to owner widget then copy them in this delegate.
	Q_PROPERTY(bool isOnlyPlainText READ isOnlyPlainText WRITE setOnlyPlainText)
	Q_PROPERTY(bool paintRoundedRect READ paintRoundedRect WRITE setPaintRoundedRect)
	Q_PROPERTY(QString waitingSVG READ waitingSVG WRITE setWaitingSVG)
	Q_PROPERTY(bool waitingSVG_Over READ waitingSVG_Over WRITE setWaitingSVG_Over)

public:
	RSElidedItemDelegate(QObject *parent = 0);

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

	bool isOnlyPlainText() const { return mOnlyPlainText; }
	void setOnlyPlainText(const bool &value) { mOnlyPlainText = value; }
	bool paintRoundedRect() const { return mPaintRoundedRect; }
	void setPaintRoundedRect(const bool &value) { mPaintRoundedRect = value; }
	QString waitingSVG() const {return mWaitingSVG; }
	void setWaitingSVG(const QString &value) {mWaitingSVG = value; }
	bool waitingSVG_Over() const {return mWaitingSVG_Over; }
	void setWaitingSVG_Over(const bool &value) {mWaitingSVG_Over = value; }

protected:
	bool editorEvent(QEvent *event,
	                 QAbstractItemModel *model,
	                 const QStyleOptionViewItem &option,
	                 const QModelIndex &index) override;

private:
	bool mOnlyPlainText;
	bool mPaintRoundedRect;
	QSvgRenderer* mSVG_Renderer;
	mutable QString mLastSVG_Loaded;
	QString mWaitingSVG;
	bool mWaitingSVG_Over;
};

#endif // RSELIDEDITEMDELEGATE_H
