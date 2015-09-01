/****************************************************************************
 **
 ** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
 ** Contact: http://www.qt-project.org/legal
 **
 ** This file is part of the QtCore module of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:BSD$
 ** You may use this file under the terms of the BSD license as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
 **     of its contributors may be used to endorse or promote products derived
 **     from this software without specific prior written permission.
 **
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

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
{
	mRectElision = QRect();
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

ElidedLabel::ElidedLabel(QWidget *parent)
  : QLabel(parent)
  , mElided(false)
  , mOnlyPlainText(false)
  , mContent("")
{
	mRectElision = QRect();
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
	QList<QPair<QTextLine,QPoint> > lLines;
	QString elidedLastLine = "";
	QPainter painter(this);
	QFontMetrics fontMetrics = painter.fontMetrics();
	QRect cr = contentsRect();
	cr.adjust(margin(), margin(), -margin(), -margin());
	
	bool didElide = false;
	QChar ellipsisChar(0x2026);//= "…"
	int lineSpacing = fontMetrics.lineSpacing();
	int y = 0;

	QString plainText = "";
	if (mOnlyPlainText)
	{
		plainText = mContent;
	} else {
		QTextDocument td;
		td.setHtml(mContent);
		plainText = td.toPlainText();
	}
	plainText = plainText.replace("\n",QChar(QChar::LineSeparator));
	plainText = plainText.replace("\r",QChar(QChar::LineSeparator));

	QTextLayout textLayout(plainText, painter.font());
	QTextOption to = textLayout.textOption();
	to.setAlignment(alignment());
	to.setWrapMode(wordWrap() ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
	textLayout.setTextOption(to);

	textLayout.beginLayout();
	forever {
		//Get new line for text.
		QTextLine line = textLayout.createLine();

		if (!line.isValid())
			break;// No more line to write

		line.setLineWidth(cr.width());
		int nextLineY = y + lineSpacing;

		if ((cr.height() >= nextLineY + lineSpacing) && wordWrap()) {
			//Line written normaly, next line will too
			lLines.append(QPair<QTextLine, QPoint>(line, QPoint(0, y)));
			y = nextLineY;
		} else {
			//The next line can't be written.
			QString lastLine = plainText.mid(line.textStart()).split(QChar(QChar::LineSeparator)).at(0);
			QTextLine lineEnd = textLayout.createLine();
			if (!lineEnd.isValid() && (wordWrap()
			                           || (fontMetrics.width(lastLine) < cr.width()))) {
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
	if (alignment() & Qt::AlignTop)
		iTransY = 0;
	if (alignment() & Qt::AlignBottom)
		iTransY = cr.height() - iHeight;
	if (alignment() & Qt::AlignVCenter)
		iTransY = (cr.height() - iHeight) / 2;

	QPair<QTextLine,QPoint> pair;
	QPoint lastPos(-1,-1);
	//Now we know how many lines to redraw at good position
	foreach (pair, lLines){
		lastPos = pair.second + QPoint(0, iTransY);
		pair.first.draw(&painter, lastPos);
	}

	//Print last elided line
	if (didElide) {
		int width = fontMetrics.width(elidedLastLine);
		if (lastPos.y() == -1){
			y = iTransY;// Only one line
		} else {
			y = lastPos.y() + lineSpacing;
		}
		if (width < cr.width()){
			//Text don't taking all line (with line break), so align it
			if (alignment() & Qt::AlignLeft)
				iTransX = 0;
			if (alignment() & Qt::AlignRight)
				iTransX = cr.width() - width;
			if (alignment() & Qt::AlignHCenter)
				iTransX = (cr.width() - width) / 2;
			if (alignment() & Qt::AlignJustify)
				iTransX = 0;
		}

		painter.drawText(QPoint(iTransX, y + fontMetrics.ascent()), elidedLastLine);
		//Draw button to get ToolTip
		mRectElision = QRect(iTransX + width - fontMetrics.width(ellipsisChar)
		                     , y
		                     , fontMetrics.width(ellipsisChar)
		                     , fontMetrics.height() - 1);
		painter.drawRoundRect(mRectElision);
	} else {
		mRectElision = QRect();
	}

	//Send signal if changed
	if (didElide != mElided) {
		mElided = didElide;
		emit elisionChanged(didElide);
	}
}

void ElidedLabel::mousePressEvent(QMouseEvent *ev)
{
	if (mElided && (ev->buttons()==Qt::LeftButton) && (mRectElision.contains(ev->pos()))){
		QToolTip::showText(mapToGlobal(QPoint(0, 0)),QString("<FONT>") + mContent + QString("</FONT>"));
	}
}
