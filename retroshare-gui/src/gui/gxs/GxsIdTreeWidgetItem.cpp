/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdTreeWidgetItem.cpp                          *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie     <retroshare.project@gmail.com>     *
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

#include "rshare.h"
#include "GxsIdTreeWidgetItem.h"
#include "GxsIdDetails.h"
#include "util/HandleRichText.h"

#define BANNED_IMAGE ":/icons/yellow_biohazard64.png"

/** Constructor */
GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,bool auto_tooltip,QTreeWidget *parent)
    : QObject(NULL), RSTreeWidgetItem(compareRole, parent), mColumn(0), mIconTypeMask(icon_mask),mAutoTooltip(auto_tooltip)
{
	init();
}

void GxsIdRSTreeWidgetItem::init()
{
	mIdFound = false;
	mRetryWhenFailed = false;
    	mBannedState = false ;
}

static void fillGxsIdRSTreeWidgetItemCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	GxsIdRSTreeWidgetItem *item = dynamic_cast<GxsIdRSTreeWidgetItem*>(object);
	if (!item) {
		return;
	}

	QList<QIcon> icons;

	switch (type) {
	case GXS_ID_DETAILS_TYPE_EMPTY:
		item->processResult(true);
		break;

	case GXS_ID_DETAILS_TYPE_FAILED:
		item->processResult(false);
		break;

	case GXS_ID_DETAILS_TYPE_LOADING:
		icons.push_back(GxsIdDetails::getLoadingIcon(details.mId));
		break;

	case GXS_ID_DETAILS_TYPE_DONE:
        GxsIdDetails::getIcons(details, icons, item->iconTypeMask());
		item->processResult(true);
		break;
        
    	case GXS_ID_DETAILS_TYPE_BANNED:
        	icons.push_back(QIcon("BANNED_IMAGE")) ;
            	break ;
	}

	int column = item->idColumn();

    if(item->autoTooltip())
		item->setToolTip(column, GxsIdDetails::getComment(details));

	item->setText(column, GxsIdDetails::getNameForType(type, details));
	item->setData(column, Qt::UserRole, QString::fromStdString(details.mId.toStdString()));

	QPixmap combinedPixmap;
	if (!icons.empty()) {
        GxsIdDetails::GenerateCombinedPixmap(combinedPixmap, icons, QFontMetricsF(item->font(item->idColumn())).height()*1.4);
	}
	item->setData(column, Qt::DecorationRole, combinedPixmap);
	item->setAvatar(details.mAvatar);
}

void GxsIdRSTreeWidgetItem::setId(const RsGxsId &id, int column, bool retryWhenFailed)
{
	//std::cerr << " GxsIdRSTreeWidgetItem::setId(" << id << "," << column << ")";
	//std::cerr << std::endl;

	if (mIdFound && mColumn == column && mId == id && (mBannedState == rsReputations->isIdentityBanned(mId)))
			return;

	mBannedState = rsReputations->isIdentityBanned(mId);
	mIdFound = false;
	mRetryWhenFailed = retryWhenFailed;

	mId = id;
	mColumn = column;

	startProcess();
}

void GxsIdRSTreeWidgetItem::updateBannedState()
{
    if(mBannedState != rsReputations->isIdentityBanned(mId))
        forceUpdate() ;
}

void GxsIdRSTreeWidgetItem::forceUpdate()
{
	mIdFound = false;
	mBannedState = (rsReputations->isIdentityBanned(mId));

	startProcess();
}

void GxsIdRSTreeWidgetItem::startProcess()
{
	if (mRetryWhenFailed) {
		disconnect(rApp, SIGNAL(minuteTick()), this, SLOT(startProcess()));
	}

	GxsIdDetails::process(mId, fillGxsIdRSTreeWidgetItemCallback, this);
}

bool GxsIdRSTreeWidgetItem::getId(RsGxsId &id)
{
	id = mId;
	return true;
}

void GxsIdRSTreeWidgetItem::processResult(bool success)
{
	mIdFound = success;

	if (!mIdFound && mRetryWhenFailed) {
		/* Try again */
		connect(rApp, SIGNAL(minuteTick()), this, SLOT(startProcess()));
	}
}

void GxsIdRSTreeWidgetItem::setAvatar(const RsGxsImage &avatar)
{
	mAvatar = avatar;
}

QVariant GxsIdRSTreeWidgetItem::data(int column, int role) const
{
	if (column == idColumn())
	{
		if (role == Qt::ToolTipRole)
		{
			QString t = RSTreeWidgetItem::data(column, role).toString();
			QPixmap pix;

			if(mId.isNull())
                return RSTreeWidgetItem::data(column, role);
			else if( rsReputations->overallReputationLevel(mId) == RsReputationLevel::LOCALLY_NEGATIVE )
				pix = QPixmap(BANNED_IMAGE);
			else if ( mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(mAvatar.mData, mAvatar.mSize, pix,GxsIdDetails::LARGE) )
				pix = GxsIdDetails::makeDefaultIcon(mId,GxsIdDetails::LARGE);

			int S = QFontMetricsF(font(column)).height();

			QString embeddedImage;

			if ( RsHtml::makeEmbeddedImage( pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation ).toImage(), embeddedImage, 8*S * 8*S ) )
				t = "<table><tr><td>" + embeddedImage + "</td><td>" + t + "</td></table>";

			return t;
		}
	}

	return RSTreeWidgetItem::data(column, role);
}

QSize GxsIdTreeItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	RsGxsId id(index.data(Qt::UserRole).toString().toStdString());

	if(id.isNull())
		return QStyledItemDelegate::sizeHint(option,index);

	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);

	// disable default icon
	opt.icon = QIcon();
	const QRect r = option.rect;
	QString str;
	QList<QIcon> icons;
	QString comment;

	QFontMetricsF fm(option.font);
	float f = fm.height();

	QIcon icon ;

	if(!GxsIdDetails::MakeIdDesc(id, true, str, icons, comment,GxsIdDetails::ICON_TYPE_AVATAR))
	{
		icon = GxsIdDetails::getLoadingIcon(id);
		launchAsyncLoading();
	}
	else
		icon = *icons.begin();

	QPixmap pix = icon.pixmap(r.size());

	return QSize(1.2*(pix.width() + fm.width(str)),std::max(1.1*pix.height(),1.4*fm.height()));
}


void GxsIdTreeItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex& index) const
{
	if(!index.isValid())
	{
		std::cerr << "(EE) attempt to draw an invalid index." << std::endl;
		return ;
	}

	RsGxsId id(index.data(Qt::UserRole).toString().toStdString());

	if(id.isNull())
		return QStyledItemDelegate::paint(painter,option,index);

	QStyleOptionViewItemV4 opt = option;
	initStyleOption(&opt, index);

	// disable default icon
	opt.icon = QIcon();
	// draw default item
	QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &opt, painter, 0);

	QRect r = option.rect;

	QString str;
	QString comment;

	QFontMetricsF fm(painter->font());
	float f = fm.height();

	QIcon icon ;

	if(id.isNull())
	{
		str = tr("[Notification]");
		icon = QIcon(":/icons/notification.png");
	}
	else if(! computeNameIconAndComment(id,str,icon,comment))
		if(mReloadPeriod > 3)
		{
			str = tr("[Unknown]");
			icon = QIcon(":/icons/anonymous.png");
		}
		else
		{
			icon = GxsIdDetails::getLoadingIcon(id);
			launchAsyncLoading();
		}

    QRect pixmaprect(r);
    pixmaprect.adjust(r.height(),0,0,0);

	QPixmap pix = icon.pixmap(pixmaprect.size());
	const QPoint p = QPoint(r.height()/2.0, (r.height() - pix.height())/2);

	// draw pixmap at center of item
	painter->drawPixmap(r.topLeft() + p, pix);

	QRect mRectElision;
    r.adjust(pix.height()+f,(r.height()-f)/2.0,0,0);

	bool didElide = ElidedLabel::paintElidedLine(*painter,str,r,Qt::AlignLeft,false,false,mRectElision);
}
