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

	/**
	 * @brief paintElidedLine: Draw elided(…) line to a painter.
	 * @param painter: wher to paint line. If null, only calculate rectElision.
	 * painter pen have to be setted before. This doesn't paint background.
	 * @param plainText: Plain text to paint.
	 * @param cr: Area where to paint. If too close, text is elided.
	 * @param font: Font to use to pain text.
	 * @param alignment: Which alignement to use for text.
	 * @param wordWrap: If it elide by char or by word.
	 * @param drawRoundRect: If rounded rect around … is need. To notice user he can click on it.
	 * @param rectElision: Where elision occurs. To manage click. Can be omitted.
	 * @return If text need to be elided.
	 */
	static bool paintElidedLine( QPainter* painter, QString plainText
	                           , const QRect& cr,  QFont font
	                           , Qt::Alignment alignment,bool wordWrap
	                           , bool drawRoundedRect,QRect* rectElision = nullptr);

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
