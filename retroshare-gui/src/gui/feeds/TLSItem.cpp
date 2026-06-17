/*******************************************************************************
 * gui/feeds/TLSItem.cpp                                                       *
 *                                                                             *
 * Copyright (c) 2026, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <QDateTime>
#include <QTimer>
#include <QMessageBox>

#include "TLSItem.h"
#include "FeedHolder.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/common/FilesDefs.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"

/*****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
TLSItem::TLSItem(FeedHolder *parent, uint32_t feedId): FeedItem(parent,feedId,NULL)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	expandFrame->hide();

    QObject::connect(expandButton,SIGNAL(clicked()),this,SLOT(toggle()));
    QObject::connect(clearButton,SIGNAL(clicked()),this,SLOT(removeItem()));

	updateItemStatic();
	updateItem();
}

TLSItem::~TLSItem()
{
}

uint64_t TLSItem::uniqueIdentifier() const
{
    return hash_64bits("TLSItem");
}

void TLSItem::addIPEntry(const std::string& ip_string)
{
    mIps.push_front(std::make_pair(QDateTime::currentDateTime(),ip_string));

    while(mIps.size() > 5) // quadratic cost, but for small lists it's ok
        mIps.pop_back();

    updateItem();
}

void TLSItem::updateItemStatic()
{
    ipLabel_2->clear();
    ip_TW->clearContents();

    if(mIps.empty())
        return;

    ipLabel_2->setText(QString::fromStdString(mIps.front().second.c_str()));
    timeLabel->setText(" ("+mIps.front().first.toString()+")");

    int r=0;

    for(const auto& lp:mIps)
    {
        ip_TW->insertRow(r);
        ip_TW->setItem(r,0,new QTableWidgetItem(QString::fromStdString(lp.second.c_str())));
        ip_TW->setItem(r,1,new QTableWidgetItem(lp.first.toString()));

        ++r;
    }
}

void TLSItem::updateItem()
{
    updateItemStatic();
}

void TLSItem::toggle()
{
	expand(expandFrame->isHidden());
}

void TLSItem::doExpand(bool open)
{
	if (mFeedHolder) {
		mFeedHolder->lockLayout(this, true);
	}

	if (open)
	{
		expandFrame->show();
        expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/up-arrow.png")));
		expandButton->setToolTip(tr("Hide"));
	}
	else
	{
		expandFrame->hide();
        expandButton->setIcon(FilesDefs::getIconFromQtResourcePath(QString(":/icons/png/down-arrow.png")));
		expandButton->setToolTip(tr("Expand"));
	}

	emit sizeChanged(this);

	if (mFeedHolder) {
		mFeedHolder->lockLayout(this, false);
	}
}


