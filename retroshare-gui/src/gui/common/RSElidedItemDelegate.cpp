/*******************************************************************************
 * gui/common/RsEdlideLabelItemDelegate.cpp                                    *
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

#include "RSElidedItemDelegate.h"

#include <QPainter>
#include <QTextDocument>
#include <QTextLayout>
#include <QToolTip>

RSElidedItemDelegate::RSElidedItemDelegate(QObject *parent)
  : RSItemDelegate(parent)
  , mElided(false)
  , mOnlyPlainText(false)
  , mContent("")
{
	mRectElision = QRect();
}

void RSElidedItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	RSItemDelegate::paint(painter, option, index);
}

void RSElidedItemDelegate::drawDisplay(QPainter *painter, const QStyleOptionViewItem &option,
                 const QRect &rect, const QString &text) const
{
	if (!text.isEmpty())
	{
		mContent = text;
		QList<QPair<QTextLine,QPoint> > lLines;
		QString elidedLastLine = "";
		QFontMetrics fontMetrics = option.fontMetrics;
		QRect cr = rect;
		cr.adjust(spacing().width(), spacing().height(), -spacing().width(), -spacing().height());

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

		if (painter) {
			painter->setFont(option.font);
			QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
			    ? QPalette::Normal : QPalette::Disabled;
			if (cg == QPalette::Normal && !(option.state & QStyle::State_Active))
				cg = QPalette::Inactive;
			QColor textColor = option.palette.color(cg, QPalette::Text);
			painter->setPen(textColor);
		}

		QTextLayout textLayout(plainText, option.font);
		QTextOption to = textLayout.textOption();
		to.setAlignment(option.displayAlignment);
#if QT_VERSION >= 0x050000
		bool wordWrap = option.features.testFlag(QStyleOptionViewItem::WrapText);
#else
		bool wordWrap = false;
#endif
		to.setWrapMode(wordWrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
		textLayout.setTextOption(to);

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
		if (option.displayAlignment & Qt::AlignTop)
			iTransY = 0;
		if (option.displayAlignment & Qt::AlignBottom)
			iTransY = cr.height() - iHeight;
		if (option.displayAlignment & Qt::AlignVCenter)
			iTransY = (cr.height() - iHeight) / 2;

		QPair<QTextLine,QPoint> pair;
		QPoint lastPos(-1,-1);
		//Now we know how many lines to redraw at good position
		foreach (pair, lLines){
			lastPos = pair.second + QPoint(0 + cr.left(), iTransY + cr.top());
			if (painter) pair.first.draw(painter, lastPos);
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
				if (option.displayAlignment & Qt::AlignLeft)
					iTransX = 0;
				if (option.displayAlignment & Qt::AlignRight)
					iTransX = cr.width() - width;
				if (option.displayAlignment & Qt::AlignHCenter)
					iTransX = (cr.width() - width) / 2;
				if (option.displayAlignment & Qt::AlignJustify)
					iTransX = 0;
			}

			if (painter) painter->drawText(QPoint(iTransX + cr.left(), y + fontMetrics.ascent() + cr.top()), elidedLastLine);
			//Draw button to get ToolTip
			mRectElision = QRect(iTransX + width - fontMetrics.width(ellipsisChar) + cr.left()
			                     , y + cr.top()
			                     , fontMetrics.width(ellipsisChar)
			                     , fontMetrics.height() - 1);
			if (painter) painter->drawRoundRect(mRectElision);

		} else {
			mRectElision = QRect();
		}

		mElided = didElide;
	}
}

bool RSElidedItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::MouseButtonPress) {
		QMouseEvent *ev = static_cast<QMouseEvent *>(event);
		if (ev) {
			if (ev->buttons()==Qt::LeftButton) {
				if (index.data().type() == QVariant::String) {
					QString text = index.data().toString();
					if (!text.isEmpty()) {
						QRect rect = option.rect;
						//Get decoration Rect to get rect == to rect sent in drawDisplay
						if (index.data(Qt::DecorationRole).type() == QVariant::Pixmap) {
							QPixmap pixmap = index.data(Qt::DecorationRole).value<QPixmap>();
							if (!pixmap.isNull()) rect.adjust(pixmap.width() + 6,0,0,0);//Don't know where come from these 6 pixels...
						}
						if (index.data(Qt::DecorationRole).type() == QVariant::Image) {
							QImage image = index.data(Qt::DecorationRole).value<QImage>();
							if (!image.isNull()) rect.adjust(image.width() + 6,0,0,0);//Don't know where come from these 6 pixels...
						}
						if (index.data(Qt::DecorationRole).type() == QVariant::Icon) {
							QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
							if (!icon.isNull()) {
								QSize size = icon.actualSize(rect.size());
								rect.adjust(size.width() + 6,0,0,0);//Don't know where come from these 6 pixels...
							}
						}
						//Get Font as option.font is not accurate
						QStyleOptionViewItem newOption = option;
						if (index.data(Qt::FontRole).type() == QVariant::Font) {
							QFont font = index.data(Qt::FontRole).value<QFont>();
							newOption.font = font;
							newOption.fontMetrics = QFontMetrics(font);
						}

						//Update RSElidedItemDelegate as only one delegate for all items
						drawDisplay(NULL, newOption, rect, text);
						if (mElided && (mRectElision.contains(ev->pos()))){
							QToolTip::showText(ev->globalPos(),QString("<FONT>") + mContent + QString("</FONT>"));
							return true; // eat event
						}
					}
				}
			}
		}
	}
	return RSItemDelegate::editorEvent(event, model, option, index);
}
