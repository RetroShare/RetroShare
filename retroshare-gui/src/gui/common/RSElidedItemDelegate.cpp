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

#include "gui/common/StyledElidedLabel.h"
#include "util/rsdebug.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QPainter>
#include <QTextDocument>
#include <QTextLayout>
#include <QToolTip>

RSElidedItemDelegate::RSElidedItemDelegate(QObject *parent)
  : RSStyledItemDelegate(parent)
  , mOnlyPlainText(false), mPaintRoundedRect(true)
  , mWaitingSVG(":/icons/svg/waiting.svg"), mWaitingSVG_Over(true)
{
	mSVG_Renderer = new QSvgRenderer(this);
}

QSize RSElidedItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QStyleOptionViewItem ownOption (option);
	initStyleOption(&ownOption, index);

	const QWidget* widget = option.widget;
	QStyle* ownStyle =  widget ? widget->style() : QApplication::style();

	//Only need "…" for text
	ownOption.text = "…";
	QRect checkRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &ownOption, widget);
	QRect iconRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemDecoration, &ownOption, widget);
	QRect textRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemText, &ownOption, widget);

	QSize contSize = ownStyle->sizeFromContents( QStyle::CT_ItemViewItem,&ownOption
	                                            ,QSize( checkRect.width()+iconRect.width()+textRect.width()
	                                                   ,qMax(checkRect.height(),qMax(iconRect.height(),textRect.height()))),widget);

	return contSize;
}

void RSElidedItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	if(!index.isValid())
	{
		RsErr() << __PRETTY_FUNCTION__ << " attempt to draw an invalid index." << std::endl;
		return ;
	}
	painter->save();
	// To draw with default for debug purpose
	//QStyledItemDelegate::paint(painter, option, index);

	QStyleOptionViewItem ownOption (option);
	initStyleOption(&ownOption, index);
	//Prefer use icon from option
	if (!option.icon.isNull())
		ownOption.icon = option.icon;

	const QWidget* widget = option.widget;
	QStyle* ownStyle =  widget ? widget->style() : QApplication::style();

	if (!mOnlyPlainText)
	{
		QTextDocument td;
		td.setHtml(ownOption.text);
		ownOption.text = td.toPlainText();
	}
	//Get Font as option.font is not accurate
	if (index.data(Qt::FontRole).type() == QVariant::Font) {
		QFont font = index.data(Qt::FontRole).value<QFont>();
		ownOption.font = font;
		ownOption.fontMetrics = QFontMetrics(font);
	}
	QColor textColor;
	if (index.data(Qt::TextColorRole).canConvert(QMetaType::QColor)) {
		textColor = QColor(index.data(Qt::TextColorRole).toString());//Needs to pass from string else loose RBG format.
	}
	if (index.data(Qt::BackgroundRole).canConvert(QMetaType::QBrush)) {
		QBrush brush(index.data(Qt::BackgroundRole).convert(QMetaType::QBrush));
		ownOption.backgroundBrush = brush;
	}

	//Code from: https://code.woboq.org/qt5/qtbase/src/widgets/styles/qcommonstyle.cpp.html#2271
	QRect checkRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &ownOption, widget);
	QRect iconRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemDecoration, &ownOption, widget);
	QRect textRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemText, &ownOption, widget);

	// draw the background
	ownStyle->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, widget);
	// draw the check mark
	if (ownOption.features & QStyleOptionViewItem::HasCheckIndicator) {
		QStyleOptionViewItem option(*&ownOption);
		option.rect = checkRect;
		option.state = option.state & ~QStyle::State_HasFocus;
		switch (ownOption.checkState) {
			case Qt::Unchecked:
				option.state |= QStyle::State_Off;
			break;
			case Qt::PartiallyChecked:
				option.state |= QStyle::State_NoChange;
			break;
			case Qt::Checked:
				option.state |= QStyle::State_On;
			break;
		}
		ownStyle->drawPrimitive(QStyle::PE_IndicatorItemViewItemCheck, &option, painter, widget);
	}
	// draw the icon
	{
		if (!ownOption.icon.isNull())
		{
			QString status;
			if (index.data(Qt::StatusTipRole).canConvert(QMetaType::QString))
				status = index.data(Qt::StatusTipRole).toString();

			if (mWaitingSVG_Over || (status.toLower() != "waiting"))
			{
				QIcon::Mode mode = QIcon::Normal;
				if (!(ownOption.state & QStyle::State_Enabled))
					mode = QIcon::Disabled;
				else if (ownOption.state & QStyle::State_Selected)
					mode = QIcon::Selected;
				QIcon::State state = ownOption.state & QStyle::State_Open ? QIcon::On : QIcon::Off;
				ownOption.icon.paint(painter, iconRect, ownOption.decorationAlignment, mode, state);
			}
			if (status.toLower() == "waiting")
			{
				if (mLastSVG_Loaded != mWaitingSVG)
					mSVG_Renderer->load(mWaitingSVG);
				mLastSVG_Loaded = mWaitingSVG;

				const QAbstractItemView* aiv = dynamic_cast<const QAbstractItemView*>(option.widget);
				if (aiv)
					connect(mSVG_Renderer, SIGNAL(repaintNeeded()), aiv->viewport(),SLOT(update()));

				mSVG_Renderer->render(painter, iconRect);
			}
		}
	}
	// draw the text
	if (!ownOption.text.isEmpty()) {
		QPalette::ColorGroup cg = ownOption.state & QStyle::State_Enabled
		        ? QPalette::Normal : QPalette::Disabled;
		if (cg == QPalette::Normal && !(ownOption.state & QStyle::State_Active))
			cg = QPalette::Inactive;
		if (ownOption.state & QStyle::State_Selected) {
			painter->setPen(ownOption.palette.color(cg, QPalette::HighlightedText));
		} else {
#if QT_VERSION >= QT_VERSION_CHECK(5,6,0)
			if (ownOption.state & QStyle::State_MouseOver) {
				//TODO: Manage to get palette with HOVER css pseudoclass
				// For now this is hidden by Qt: https://code.woboq.org/qt5/qtbase/src/widgets/styles/qstylesheetstyle.cpp.html#6103
				// So we print default in image and get it's color...
				QSize moSize=sizeHint(option,index);
				QImage moImg(moSize,QImage::Format_ARGB32);
				QPainter moPnt;
				moPnt.begin(&moImg);
				moPnt.setPen(Qt::black);//Fill Image with Black
				moPnt.setBrush(Qt::black);
				moPnt.drawRect(moImg.rect());

				QStyleOptionViewItem moOption (option);
				// Define option to get only what we want
				{
					moOption.rect = QRect(QPoint(0,0),moSize);
					moOption.state = QStyle::State_MouseOver | QStyle::State_Enabled | QStyle::State_Sibling;
					moOption.text = "████████████████";
					// Remove unwanted info. Yes it can draw without that all public data ...
					moOption.backgroundBrush = QBrush();
					moOption.checkState = Qt::Unchecked;
					moOption.decorationAlignment = Qt::AlignLeft;
					moOption.decorationPosition = QStyleOptionViewItem::Left;
					moOption.decorationSize = QSize();
					moOption.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
					moOption.features=0;
					moOption.font = QFont();
					moOption.icon = QIcon();
					moOption.index = QModelIndex();
					moOption.locale = QLocale();
					moOption.showDecorationSelected = false;
					moOption.textElideMode = Qt::ElideNone;
					moOption.viewItemPosition = QStyleOptionViewItem::Middle;
					//moOption.widget = nullptr; //Needed.

					moOption.direction = Qt::LayoutDirectionAuto;
					moOption.fontMetrics = QFontMetrics(QFont());
					moOption.palette = QPalette();
					moOption.styleObject = nullptr;
				}
				QStyledItemDelegate::paint(&moPnt, moOption, QModelIndex());

				//// But these lines doesn't works.
				{
					//QStyleOptionViewItem moOptionsState;
					//moOptionsState.initFrom(moOption.widget);
					//moOptionsState.rect = QRect(QPoint(0,0),moSize);
					//moOptionsState.state = QStyle::State_MouseOver | QStyle::State_Enabled | QStyle::State_Sibling;
					//moOptionsState.text = "████████";
					//moOptionsState.widget = option.widget;
					//QStyledItemDelegate::paint(&moPnt, moOptionsState, QModelIndex());
				}

				moPnt.end();
				// To save what it paint
				//moImg.save("image.bmp");

				// Get Color in this black rect.
				QColor moColor;
				QColor moColorBorder;// To avoid Border pixel
				int moWidth = moImg.size().width(), moHeight = moImg.size().height();
				for (int x = 0; (x<moWidth) && (moColor.spec() == QColor::Invalid); x++)
					for (int y = 0; (y<moHeight) && (moColor.spec() == QColor::Invalid); y++)
						if (moImg.pixelColor(x,y) != Qt::black)
						{
							if (moImg.pixelColor(x,y) == moColorBorder)
								moColor = QColor(moImg.pixelColor(x,y).name());
							else
							{
								if (moColorBorder.spec() == QColor::Invalid)
								{
									// First pixel border move inside
									x+=5;
									y+=5;
								}
								moColorBorder = QColor(moImg.pixelColor(x,y).name());
							}
						}

				// If not found color is black too.
				if (moColor.spec() == QColor::Invalid)
					moColor = Qt::black;

				painter->setPen(moColor);
			}
			else
#endif
				if (textColor.spec()==QColor::Invalid) {
					painter->setPen(ownOption.palette.color(cg, QPalette::Text));
				} else { //Only get color from index for unselected(as Qt does)
					painter->setPen(textColor);
				}
		}
		if (ownOption.state & QStyle::State_Editing) {
			painter->setPen(ownOption.palette.color(cg, QPalette::Text));
			painter->drawRect(textRect.adjusted(0, 0, -1, -1));
		}
		//d->viewItemDrawText(p, &ownOption, textRect);
		QTextLayout textLayout(ownOption.text, painter->font());
		QTextOption to = textLayout.textOption();
		StyledElidedLabel::paintElidedLine(painter,ownOption.text,textRect,ownOption.font,ownOption.displayAlignment,to.wrapMode()&QTextOption::WordWrap,mPaintRoundedRect);
	}
	painter->restore();
}

bool RSElidedItemDelegate::editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
	if (event->type() == QEvent::MouseButtonPress) {
		QMouseEvent *ev = static_cast<QMouseEvent *>(event);
		if (ev) {
			if (ev->buttons()==Qt::LeftButton) {
				QVariant var = index.data();
				if (index.data().type() == QVariant::String) {
					QString text = index.data().toString();
					if (!text.isEmpty()) {

						QStyleOptionViewItem ownOption (option);
						initStyleOption(&ownOption, index);

						const QWidget* widget = option.widget;
						QStyle* ownStyle =  widget ? widget->style() : QApplication::style();

						if (!mOnlyPlainText)
						{
							QTextDocument td;
							td.setHtml(ownOption.text);
							ownOption.text = td.toPlainText();
						}
						//Get Font as option.font is not accurate
						if (index.data(Qt::FontRole).type() == QVariant::Font) {
							QFont font = index.data(Qt::FontRole).value<QFont>();
							ownOption.font = font;
							ownOption.fontMetrics = QFontMetrics(font);
						}
						QRect textRect = ownStyle->subElementRect(QStyle::SE_ItemViewItemText, &ownOption, widget);

						QTextLayout textLayout(text, ownOption.font);
						QTextOption to = textLayout.textOption();

						//Update RSElidedItemDelegate as only one delegate for all items
						QRect rectElision;
						bool elided = StyledElidedLabel::paintElidedLine(nullptr,text,textRect,ownOption.font,ownOption.displayAlignment,to.wrapMode()&QTextOption::WordWrap,true,&rectElision);
						if (elided && (rectElision.contains(ev->pos()))){
							QToolTip::showText(ev->globalPos(),QString("<FONT>") + text + QString("</FONT>"));
							return true; // eat event
						}
					}
				}
			}
		}
	}
	return RSStyledItemDelegate::editorEvent(event, model, option, index);
}
