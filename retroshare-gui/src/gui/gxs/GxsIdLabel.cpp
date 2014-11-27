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

#include "GxsIdLabel.h"
#include "GxsIdDetails.h"

/** Constructor */
GxsIdLabel::GxsIdLabel(QWidget *parent)
    : QLabel(parent)
{
}

static void fillLabelCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	QLabel *label = dynamic_cast<QLabel*>(object);
	if (!label) {
		return;
	}

	label->setText(GxsIdDetails::getNameForType(type, details));

	QString toolTip;

	switch (type) {
	case GXS_ID_DETAILS_TYPE_EMPTY:
	case GXS_ID_DETAILS_TYPE_LOADING:
	case GXS_ID_DETAILS_TYPE_FAILED:
		break;

	case GXS_ID_DETAILS_TYPE_DONE:
		toolTip = GxsIdDetails::getComment(details);
		break;
	}

	label->setToolTip(toolTip);
}

void GxsIdLabel::setId(const RsGxsId &id)
{
	mId = id;

	GxsIdDetails::process(mId, fillLabelCallback, this);
}

bool GxsIdLabel::getId(RsGxsId &id)
{
	id = mId;
	return true;
}
