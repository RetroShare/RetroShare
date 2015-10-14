/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "rshare.h"
#include "GxsIdTreeWidgetItem.h"
#include "GxsIdDetails.h"
#include "util/HandleRichText.h"
#include "retroshare/rsreputations.h"

#define BANNED_IMAGE ":/icons/yellow_biohazard64.png"

/** Constructor */
GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,QTreeWidget *parent)
    : QObject(NULL), RSTreeWidgetItem(compareRole, parent), mColumn(0), mIconTypeMask(icon_mask)
{
	init();
}

GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,QTreeWidgetItem *parent)
    : QObject(NULL), RSTreeWidgetItem(compareRole, parent), mColumn(0), mIconTypeMask(icon_mask)
{
	init();
}

void GxsIdRSTreeWidgetItem::init()
{
	mIdFound = false;
	mRetryWhenFailed = false;
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

	if (mIdFound) {
		if (mColumn == column && mId == id) {
			return;
		}
	}

	mIdFound = false;
	mRetryWhenFailed = retryWhenFailed;

	mId = id;
	mColumn = column;

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
		    QImage pix;

		    if(mId.isNull())
			    return RSTreeWidgetItem::data(column, role);
		    else if(rsReputations->isIdentityBanned(mId))
			    pix = QImage(BANNED_IMAGE) ;
		    else if (mAvatar.mSize == 0 || !pix.loadFromData(mAvatar.mData, mAvatar.mSize, "PNG")) 
			    pix = GxsIdDetails::makeDefaultIcon(mId);

		    int S = QFontMetricsF(font(column)).height();

		    QString embeddedImage;
		    if (RsHtml::makeEmbeddedImage(pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation), embeddedImage, 8*S * 8*S)) {
			    t = "<table><tr><td>" + embeddedImage + "</td><td>" + t + "</td></table>";
		    }

		    return t;
	    }
    }

    return RSTreeWidgetItem::data(column, role);
}
