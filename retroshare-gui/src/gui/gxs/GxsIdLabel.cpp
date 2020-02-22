/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsIdLabel.cpp                                   *
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

#include "GxsIdLabel.h"
#include "GxsIdDetails.h"

/** Constructor */
GxsIdLabel::GxsIdLabel(bool show_tooltip,QWidget *parent)
    : QLabel(parent),mShowTooltip(show_tooltip)
{
}

static void fillLabelCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	GxsIdLabel *label = dynamic_cast<GxsIdLabel*>(object);
	if (!label) {
		return;
	}

	label->setText(GxsIdDetails::getNameForType(type, details));

    if(label->showTooltip())
	{
		QString toolTip;

		switch (type) {
		case GXS_ID_DETAILS_TYPE_EMPTY:
		case GXS_ID_DETAILS_TYPE_LOADING:
		case GXS_ID_DETAILS_TYPE_FAILED:
		case GXS_ID_DETAILS_TYPE_BANNED:
			break;

		case GXS_ID_DETAILS_TYPE_DONE:
			toolTip = GxsIdDetails::getComment(details);
			break;
		}

		label->setToolTip(toolTip);
	}
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
