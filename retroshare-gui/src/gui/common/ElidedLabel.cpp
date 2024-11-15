/*******************************************************************************
 * gui/common/ElidedLabel.cpp                                                  *
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

#include "ElidedLabel.h"

#include <QDebug>
#include <QPainter>
#include <QStyleOption>
#include <QTextDocument>
#include <QTextLayout>
#include <QToolTip>

ElidedLabel::ElidedLabel(const QString &text, QWidget *parent)
  : QLabel(parent)
  , mElided(false)
  , mOnlyPlainText(false)
  , mContent(text)
  , mRectElision(QRect())
  , mTextColor(QColor())
{
	setStyleSheet("ElidedLabel{background-color: rgba(0,0,0,0%)}");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(QWidget *parent)
  : QLabel(parent)
  , mElided(false)
  , mOnlyPlainText(false)
  , mContent("")
  , mRectElision(QRect())
  , mTextColor(QColor())
{
	setStyleSheet("ElidedLabel{background-color: rgba(0,0,0,0%)}");
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void ElidedLabel::setText(const QString &newText)
{
	mContent = newText;
	update();
}

void ElidedLabel::setOnlyPlainText(const bool &value)
{
	mOnlyPlainText = value;
	update();
}

void ElidedLabel::clear()
{
	mContent.clear();
	update();
}

void ElidedLabel::paintEvent(QPaintEvent *event)
{
	QLabel::paintEvent(event);

	QPainter painter(this);
 	QString plainText = "";
	if (mOnlyPlainText)
	{
		plainText = mContent;
	} else {
		QTextDocument td;
		td.setHtml(mContent);
		plainText = td.toPlainText();
	}
	QRect cr(contentsRect());
	cr.adjust(margin(), margin(), -margin(), -margin());

	bool didElide = paintElidedLine(&painter,plainText,cr,font(),alignment(),wordWrap(),true,&mRectElision);

	//Send signal if changed

	if (didElide != mElided)
	{
		mElided = didElide;
		emit elisionChanged(didElide);
	}
}

bool ElidedLabel::paintElidedLine( QPainter* painter, QString plainText
                                 , const QRect& cr, QFont useFont
                                 , Qt::Alignment alignment, bool wordWrap
                                 , bool drawRoundedRect, QRect* rectElision/*=nullptr*/)
{
	if (rectElision) *rectElision = QRect();
	if (plainText.isEmpty())
		return false;

	QList<QPair<QTextLine,QPoint> > lLines;
	QString elidedLastLine = "";
	QFontMetrics fontMetrics = QFontMetrics(useFont);

	bool didElide = false;
	QChar ellipsisChar(0x2026);//= "…"
	int lineSpacing = fontMetrics.lineSpacing();
	int y = 0;

	plainText = plainText.replace("\n",QChar(QChar::LineSeparator));
	plainText = plainText.replace("\r",QChar(QChar::LineSeparator));

	QTextLayout textLayout(plainText, useFont);
	QTextOption to = textLayout.textOption();
	to.setAlignment(alignment);
	to.setWrapMode(wordWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
	textLayout.setTextOption(to);

	if (painter)
	{
		useFont.setPointSize(useFont.pointSize()); //Modify it to be copied in painter. Else painter keep defaut font size.
		painter->save();
		painter->setFont(useFont);
	}
	textLayout.beginLayout();
	forever {
		//Get new line for text.
		QTextLine line = textLayout.createLine();

		if (!line.isValid())
			break;// No more line to write

		line.setLineWidth(cr.width());
		int nextLineY = y + lineSpacing;

		if ((cr.height() >= nextLineY + lineSpacing) && wordWrap) {
			//Line written normaly, next line will too
			lLines.append(QPair<QTextLine, QPoint>(line, QPoint(0, y)));
			y = nextLineY;
		} else {
			//The next line can't be written.
			QString lastLine = plainText.mid(line.textStart()).split(QChar(QChar::LineSeparator)).at(0);
			QTextLine lineEnd = textLayout.createLine();
			if (!lineEnd.isValid() && (wordWrap
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
			                           || (fontMetrics.width(lastLine) < cr.width()) ))
#else
			                           || (fontMetrics.horizontalAdvance(lastLine) < cr.width()) ))
#endif
			{
				//No more text for next line so this one is OK
				lLines.append(QPair<QTextLine, QPoint>(line, QPoint(0, y)));
				elidedLastLine="";
				didElide = false;
			} else {
				//Text is left, so get elided text
				if (lastLine == "") {
					elidedLastLine = ellipsisChar;
				} else {
					elidedLastLine = fontMetrics.elidedText(lastLine, Qt::ElideRight, cr.width()-1);
					if (elidedLastLine.right(1) != ellipsisChar)
						elidedLastLine.append(ellipsisChar);//New line at end
				}
				didElide = true;
				break;
			}
		}
	}
	textLayout.endLayout();

	int iTransX, iTransY = iTransX = 0;
	int iHeight = lLines.count() * lineSpacing;
	if (didElide) iHeight += lineSpacing;

	//Compute lines translation with alignment
	if (alignment & Qt::AlignTop)
		iTransY = 0;
	if (alignment & Qt::AlignBottom)
		iTransY = cr.height() - iHeight;
	if (alignment & Qt::AlignVCenter)
		iTransY = (cr.height() - iHeight) / 2;

	QPair<QTextLine,QPoint> pair;
	QPoint lastPos(-1,-1);
	//Now we know how many lines to redraw at good position
	foreach (pair, lLines){
		lastPos = pair.second + QPoint(0+ cr.left(), iTransY + cr.top());
		if (painter) pair.first.draw(painter, lastPos);
	}

	//Print last elided line
	if (didElide)
	{
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
		int width = fontMetrics.width(elidedLastLine);
#else
		int width = fontMetrics.horizontalAdvance(elidedLastLine);
#endif
		if (lastPos.y() == -1){
			y = iTransY;// Only one line
		} else {
			y = lastPos.y() + lineSpacing;
		}
		if (width < cr.width()){
			//Text don't taking all line (with line break), so align it
			if (alignment & Qt::AlignLeft)
				iTransX = 0;
			if (alignment & Qt::AlignRight)
				iTransX = cr.width() - width;
			if (alignment & Qt::AlignHCenter)
				iTransX = (cr.width() - width) / 2;
			if (alignment & Qt::AlignJustify)
				iTransX = 0;
		}

		if(width+iTransX+cr.left() <= cr.right())
			if (painter)
				painter->drawText(QPoint(iTransX + cr.left(), y + fontMetrics.ascent() + cr.top()), elidedLastLine);

		//Draw button to get ToolTip
#if QT_VERSION < QT_VERSION_CHECK(5,11,0)
		int fontWidth = fontMetrics.width(ellipsisChar);
#else
		int fontWidth = fontMetrics.horizontalAdvance(ellipsisChar);
#endif
		QRect mRectElision = QRect(  iTransX + width - fontWidth + cr.left()
		                           , y + cr.top()
		                           , fontWidth
		                           , fontMetrics.height() - 1);
		if (rectElision)
			*rectElision = mRectElision;

		if(drawRoundedRect)
			if (painter) {
				painter->setBrush(QBrush(Qt::transparent));
				painter->drawRoundedRect(mRectElision, 2 , 2);
			}
	}

	if (painter) painter->restore();
	return didElide;
}

void ElidedLabel::mousePressEvent(QMouseEvent *ev)
{
	if (mElided && (ev->buttons()==Qt::LeftButton) && (mRectElision.contains(ev->pos()))){
		QToolTip::showText(mapToGlobal(QPoint(0, 0)),QString("<FONT>") + mContent + QString("</FONT>"));
		return; // eat event
	}
	QLabel::mousePressEvent(ev);

    if(ev->buttons()==Qt::LeftButton)
        emit clicked(ev->pos());
    else if(ev->buttons()==Qt::RightButton)
        emit rightClicked(ev->pos());
}

void ElidedLabel::setTextColor(const QColor &color)
{
	QPalette tmpPalette = palette();
	tmpPalette.setColor(foregroundRole(), color);
	setPalette(tmpPalette);
	mTextColor = color;
}
