/*******************************************************************************
 * gui/common/ElidedLabel.h                                                    *
 *                                                                             *
 * Copyright (C) 2012, Retroshare Team <retroshare.project@gmail.com>          *
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

// Inspired from Qt examples set

#ifndef ELIDEDLABEL_H
#define ELIDEDLABEL_H

#include <QLabel>
#include <QRect>
#include <QResizeEvent>
#include <QString>
#include <QWidget>

class ElidedLabel : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(QString text READ text WRITE setText)
	Q_PROPERTY(bool isElided READ isElided)
	Q_PROPERTY(bool isOnlyPlainText READ isOnlyPlainText WRITE setOnlyPlainText)
	Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)

public:
	ElidedLabel(const QString &text, QWidget *parent = 0);
	ElidedLabel(QWidget *parent = 0);

	const QString & text() const { return mContent; }
	bool isElided() const { return mElided; }
	bool isOnlyPlainText() const { return mOnlyPlainText; }

	QColor textColor() const { return mTextColor; }
	void setTextColor(const QColor &color);

	static bool paintElidedLine(QPainter& painter, QString plainText, const QRect &cr, Qt::Alignment alignment, bool wordWrap, bool drawRoundRect, QRect &rectElision);

public slots:
	void setText(const QString &text);
	void setOnlyPlainText(const bool &value);
	void clear();

protected:
	void paintEvent(QPaintEvent *event);
	void mousePressEvent(QMouseEvent *ev);

signals:
	void elisionChanged(bool elided);

private:
	bool mElided;
	bool mOnlyPlainText;
	QString mContent;
	QRect mRectElision;
	QColor mTextColor;
};

#endif // ELIDEDLABEL_H
