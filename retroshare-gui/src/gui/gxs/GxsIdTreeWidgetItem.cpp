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
GxsIdRSTreeWidgetItem::GxsIdRSTreeWidgetItem(const RSTreeWidgetItemCompareRole *compareRole, uint32_t icon_mask,QTreeWidget *parent)
    : QObject(NULL), RSTreeWidgetItem(compareRole, parent), mColumn(0), mIconTypeMask(icon_mask)
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
			else if ( mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(mAvatar.mData, mAvatar.mSize, pix) )
				pix = GxsIdDetails::makeDefaultIcon(mId);

			int S = QFontMetricsF(font(column)).height();

			QString embeddedImage;

			if ( RsHtml::makeEmbeddedImage( pix.scaled(QSize(4*S,4*S), Qt::KeepAspectRatio, Qt::SmoothTransformation ).toImage(), embeddedImage, 8*S * 8*S ) )
				t = "<table><tr><td>" + embeddedImage + "</td><td>" + t + "</td></table>";

			return t;
		}
	}

	return RSTreeWidgetItem::data(column, role);
}
