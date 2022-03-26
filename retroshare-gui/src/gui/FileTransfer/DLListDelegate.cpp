/*******************************************************************************
 * retroshare-gui/src/gui/FileTransfer/DLListDelegate.cpp                      *
 *                                                                             *
 * Copyright 2007 by Crypton         <retroshare.project@gmail.com>            *
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

#include <retroshare/rstypes.h>
#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionProgressBarV2>
#include <QProgressBar>
#include <QApplication>
#include <QDateTime>
#include <limits>
#include <math.h>

#include "DLListDelegate.h"

Q_DECLARE_METATYPE(FileProgressInfo)

// Defines for download list list columns
#define DLLISTDELEGATE_COLUMN_NAME         0
#define DLLISTDELEGATE_COLUMN_SIZE         1
#define DLLISTDELEGATE_COLUMN_COMPLETED    2
#define DLLISTDELEGATE_COLUMN_DLSPEED      3
#define DLLISTDELEGATE_COLUMN_PROGRESS     4
#define DLLISTDELEGATE_COLUMN_SOURCES      5
#define DLLISTDELEGATE_COLUMN_STATUS       6
#define DLLISTDELEGATE_COLUMN_PRIORITY     7
#define DLLISTDELEGATE_COLUMN_REMAINING    8
#define DLLISTDELEGATE_COLUMN_DOWNLOADTIME 9
#define DLLISTDELEGATE_COLUMN_ID          10
#define DLLISTDELEGATE_COLUMN_LASTDL      11
#define DLLISTDELEGATE_COLUMN_PATH        12
#define DLLISTDELEGATE_COLUMN_COUNT       13

#define PRIORITY_NULL     0.0
#define PRIORITY_FASTER   0.1
#define PRIORITY_AVERAGE  0.2
#define PRIORITY_SLOWER   0.3

#define MAX_CHAR_TMP 128

DLListDelegate::DLListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

void DLListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString byteUnits[4] = {tr("B"), tr("KB"), tr("MB"), tr("GB")};
	QStyleOptionViewItem opt = option;
	QStyleOptionProgressBarV2 newopt;
	QRect pixmapRect;
	QPixmap pixmap;
	qlonglong fileSize;
	double dlspeed, multi;
	int seconds,minutes, hours, days;
	qlonglong remaining;
    QString temp ;
	qlonglong completed;
	qlonglong downloadtime;
    qint64 qi64Value;

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

        // draw the background color if not the progress column or if progress is not displayed
        if(index.column() != DLLISTDELEGATE_COLUMN_PROGRESS) {
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
        case DLLISTDELEGATE_COLUMN_SIZE:
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
        case DLLISTDELEGATE_COLUMN_REMAINING:
			remaining = index.data().toLongLong();
			if(remaining <= 0){
        temp = "";
			} else {
				multi = 1.0;
				for(int i = 0; i < 4; ++i) {
					if (remaining < 1024) {
						remaining = index.data().toLongLong();
						temp.sprintf("%.2f ", remaining / multi);
						temp += byteUnits[i];
						break;
					}
					remaining /= 1024;
					multi *= 1024.0;
				}
			}
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
        case DLLISTDELEGATE_COLUMN_COMPLETED:
			completed = index.data().toLongLong();
                        if(completed <= 0){
                                temp = "";
			} else {
				multi = 1.0;
				for(int i = 0; i < 4; ++i) {
					if (completed < 1024) {
						completed = index.data().toLongLong();
						temp.sprintf("%.2f ", completed / multi);
						temp += byteUnits[i];
						break;
					}
					completed /= 1024;
					multi *= 1024.0;
				}
			}
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
        case DLLISTDELEGATE_COLUMN_DLSPEED:
                        dlspeed = index.data().toDouble();
                        if (dlspeed <= 0) {
                            temp = "";
                        } else {
                            temp.clear();
                            temp.sprintf("%.2f", dlspeed/1024.);
                            temp += " KB/s";
                        }
			painter->drawText(option.rect, Qt::AlignRight, temp);
			break;
        case DLLISTDELEGATE_COLUMN_PROGRESS:
			{
				// create a xProgressBar
				FileProgressInfo pinfo = index.data(Qt::UserRole).value<FileProgressInfo>() ;

//				std::cerr << "drawing progress info: nb_chunks = " << pinfo.nb_chunks ;
//				for(uint i=0;i<pinfo.cmap._map.size();++i)
//					std::cerr << pinfo.cmap._map[i] << " " ;
//				std::cerr << std::endl ;
				
				painter->save() ;
				xProgressBar progressBar(pinfo,option.rect, painter); // the 3rd param is the  color schema (0 is the default value)
				if(pinfo.type == FileProgressInfo::DOWNLOAD_LINE)
				{
					progressBar.setDisplayText(true); // should display % text?
					progressBar.setColorSchema(0) ;
				}
				else
				{
					progressBar.setDisplayText(false); // should display % text?
					progressBar.setColorSchema(1) ;
				}
				progressBar.setVerticalSpan(1);
				progressBar.paint(); // paint the progress bar

				painter->restore() ;
			}
			painter->drawText(option.rect, Qt::AlignCenter, newopt.text);
			break;
		case DLLISTDELEGATE_COLUMN_SOURCES:
		{
			double dblValue = index.data().toDouble();

			temp = dblValue!=0 ? QString("%1 (%2)").arg((int)dblValue).arg((int)((fmod(dblValue,1)*1000)+0.5)) : "";
			painter->drawText(option.rect, Qt::AlignCenter, temp);
		}
		break;
		case DLLISTDELEGATE_COLUMN_PRIORITY:
		{
			double dblValue = index.data().toDouble();
			if (dblValue == PRIORITY_NULL)
				temp = "";
			else if (dblValue == PRIORITY_FASTER)
				temp = tr("Faster");
			else if (dblValue == PRIORITY_AVERAGE)
				temp = tr("Average");
			else if (dblValue == PRIORITY_SLOWER)
				temp = tr("Slower");
			else
				temp = QString::number((uint32_t)dblValue);

			painter->drawText(option.rect, Qt::AlignCenter, temp);
		}
		break;
		case DLLISTDELEGATE_COLUMN_DOWNLOADTIME:
			downloadtime = index.data().toLongLong();
			minutes = downloadtime / 60;
			seconds = downloadtime % 60;
			hours = minutes / 60;
			minutes = minutes % 60 ;
			days = hours / 24;
			hours = hours % 24 ;
			if(days > 0) {
				temp = QString::number(days)+"d "+QString::number(hours)+"h" ;
			} else if(hours > 0 || days > 0) {
				temp = QString::number(hours)+"h "+QString::number(minutes)+"m" ;
			} else if(minutes > 0 || hours > 0) {
				temp = QString::number(minutes)+"m"+QString::number(seconds)+"s" ;
			} else if(seconds > 0) {
				temp = QString::number(seconds)+"s" ;
			} else
				temp = "" ;
			painter->drawText(option.rect, Qt::AlignCenter, temp);
			break;
		case DLLISTDELEGATE_COLUMN_NAME:
		{
			// decoration
			int pixOffset = 0;
			value = index.data(Qt::StatusTipRole);
			temp = index.data().toString();
			pixmap = qvariant_cast<QIcon>(value).pixmap(option.decorationSize, option.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled, option.state & QStyle::State_Open ? QIcon::On : QIcon::Off);
			pixmapRect = (pixmap.isNull() ? QRect(0, 0, 0, 0): QRect(QPoint(0, 0), option.decorationSize));
			if (pixmapRect.isValid()){
				QPoint p = QStyle::alignedRect(option.direction, Qt::AlignLeft, pixmap.size(), option.rect).topLeft();
				p.setX( p.x() + pixOffset);
				painter->drawPixmap(p, pixmap);
				temp = " " + temp;
				pixOffset += pixmap.size().width();
			}
			value = index.data(Qt::DecorationRole);
			temp = index.data().toString();
			pixmap = qvariant_cast<QIcon>(value).pixmap(option.decorationSize, option.state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled, option.state & QStyle::State_Open ? QIcon::On : QIcon::Off);
			pixmapRect = (pixmap.isNull() ? QRect(0, 0, 0, 0): QRect(QPoint(0, 0), option.decorationSize));
			if (pixmapRect.isValid()){
				QPoint p = QStyle::alignedRect(option.direction, Qt::AlignLeft, pixmap.size(), option.rect).topLeft();
				p.setX( p.x() + pixOffset);
				painter->drawPixmap(p, pixmap);
				temp = " " + temp;
				pixOffset += pixmap.size().width();
			}
			painter->drawText(option.rect.translated(pixOffset, 0), Qt::AlignLeft, temp);
		}
		break;
    case DLLISTDELEGATE_COLUMN_LASTDL:
        if (index.data().value<QString>().isEmpty())
            break;
        qi64Value = index.data().value<qint64>();
        if (qi64Value < std::numeric_limits<qint64>::max()){
            QDateTime qdtLastDL = QDateTime::fromTime_t(qi64Value);
            painter->drawText(option.rect, Qt::AlignCenter, qdtLastDL.toString("yyyy-MM-dd_hh:mm:ss"));
        } else {
            painter->drawText(option.rect, Qt::AlignCenter, tr("File Never Seen"));
        }
        break;
		default:
			painter->drawText(option.rect, Qt::AlignCenter, index.data().toString());
	}

	// done
	painter->restore();
}

QSize DLListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index) const
{
    float w = QFontMetricsF(option.font).width(index.data(Qt::DisplayRole).toString());

    int S = QFontMetricsF(option.font).height() ;
    return QSize(w,S);
}

