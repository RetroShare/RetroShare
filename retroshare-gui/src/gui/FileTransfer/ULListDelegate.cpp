/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/ULListDelegate.cpp                      *
 *                                                                             *
 * Copyright (c) 2007 Crypton         <retroshare.project@gmail.com>           *
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

#include <QFileInfo>

#include "ULListDelegate.h"
#include "xprogressbar.h"

Q_DECLARE_METATYPE(FileProgressInfo)

// Defines for upload list list columns
#define ULLISTDELEGATE_COLUMN_UNAME        0
#define ULLISTDELEGATE_COLUMN_UPEER        1
#define ULLISTDELEGATE_COLUMN_USIZE        2
#define ULLISTDELEGATE_COLUMN_UTRANSFERRED 3
#define ULLISTDELEGATE_COLUMN_ULSPEED      4
#define ULLISTDELEGATE_COLUMN_UPROGRESS    5
#define ULLISTDELEGATE_COLUMN_UHASH        6
#define ULLISTDELEGATE_COLUMN_UCOUNT       7

#define MAX_CHAR_TMP 128

ULListDelegate::ULListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
	;
}

ULListDelegate::~ULListDelegate(void)
{
	;
}

void ULListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString byteUnits[4] = {tr("B"), tr("KB"), tr("MB"), tr("GB")};
	QStyleOptionViewItem opt = option;
	QStyleOptionProgressBarV2 newopt;
	QRect pixmapRect;
	QPixmap pixmap;
	qlonglong fileSize;
	double ulspeed, multi;
	QString temp;
	qlonglong transferred;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	//set text color
	QVariant value = index.data(Qt::TextColorRole);
	if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));
	}
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(option.state & QStyle::State_Selected){
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	} else {
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the background color
	bool bDrawBackground = true;
	if(index.column() == ULLISTDELEGATE_COLUMN_UPROGRESS) {
		FileProgressInfo pinfo = index.data().value<FileProgressInfo>() ;
		bDrawBackground = (pinfo.type == FileProgressInfo::UNINIT);
	}
	if( bDrawBackground ) {
		if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
			if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
				cg = QPalette::Inactive;
			}
			painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
		} else {
			value = index.data(Qt::BackgroundRole);
			if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
				painter->fillRect(option.rect, qvariant_cast<QColor>(value));
			}
		}
	}

	switch(index.column()) {
        case ULLISTDELEGATE_COLUMN_USIZE:
			fileSize = index.data().toLongLong();
                        if(fileSize <= 0){
                                temp = "";
			} else {
				multi = 1.0;
				for(int i = 0; i < 4; ++i) {
					if (fileSize < 1024) {
						fileSize = index.data().toLongLong();
						temp.sprintf("%.2f ", fileSize / multi);
						temp += byteUnits[i];
						break;
					}
					fileSize /= 1024;
					multi *= 1024.0;
				}
			}
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
        case ULLISTDELEGATE_COLUMN_UTRANSFERRED:
			transferred = index.data().toLongLong();
                        if(transferred <= 0){
                                temp = "";
			} else {
				multi = 1.0;
				for(int i = 0; i < 4; ++i) {
					if (transferred < 1024) {
						transferred = index.data().toLongLong();
						temp.sprintf("%.2f ", transferred / multi);
						temp += byteUnits[i];
						break;
					}
					transferred /= 1024;
					multi *= 1024.0;
				}
			}
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
        case ULLISTDELEGATE_COLUMN_ULSPEED:
                        ulspeed = index.data().toDouble();
                        if (ulspeed <= 0) {
                            temp = "";
                        } else {
                            temp.clear();
                            temp.sprintf("%.2f", ulspeed/1024.);
                            temp += " KB/s";
                        }
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
		case ULLISTDELEGATE_COLUMN_UPROGRESS:
			{
				FileProgressInfo pinfo = index.data().value<FileProgressInfo>() ;
				if (pinfo.type == FileProgressInfo::UNINIT)
					break;

				// create a xProgressBar
				painter->save() ;
				xProgressBar progressBar(pinfo,option.rect,painter,0);// the 3rd param is the  color schema (0 is the default value)

				QString ext = QFileInfo(QString::fromStdString(index.sibling(index.row(), ULLISTDELEGATE_COLUMN_UNAME).data().toString().toStdString())).suffix();;
				if (ext == "rsfc" || ext == "rsrl" || ext == "dist" || ext == "rsfb")
					progressBar.setColorSchema( 9);
				else
					progressBar.setColorSchema( 8);

				progressBar.setDisplayText(true); // should display % text?
				progressBar.setVerticalSpan(1);
				progressBar.paint(); // paint the progress bar

				painter->restore() ;
			}
			painter->drawText(option.rect, Qt::AlignCenter, newopt.text);
			break;
		case ULLISTDELEGATE_COLUMN_UNAME:
		case ULLISTDELEGATE_COLUMN_UPEER:
			// decoration
			value = index.data(Qt::DecorationRole);
			pixmap = qvariant_cast<QIcon>(value).pixmap(option.decorationSize, option.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled, option.state & QStyle::State_Open ? QIcon::On : QIcon::Off);
			pixmapRect = (pixmap.isNull() ? QRect(0, 0, 0, 0): QRect(QPoint(0, 0), option.decorationSize));
			if (pixmapRect.isValid()){
				QPoint p = QStyle::alignedRect(option.direction, Qt::AlignLeft, pixmap.size(), option.rect).topLeft();
				painter->drawPixmap(p, pixmap);
			}
			painter->drawText(option.rect.translated(pixmap.size().width(), 0), Qt::AlignLeft, index.data().toString());
			break;
		default:
			painter->drawText(option.rect, Qt::AlignCenter, index.data().toString());
	}

	// done
	painter->restore();
}

QSize ULListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    float w = QFontMetricsF(option.font).width(index.data(Qt::DisplayRole).toString());

    int S = QFontMetricsF(option.font).height() ;
    return QSize(w,S);
}

