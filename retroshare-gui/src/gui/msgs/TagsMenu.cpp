/*******************************************************************************
 * retroshare-gui/src/gui/msgs/TagsMenu.cpp                                    *
 *                                                                             *
 * Copyright (C) 2007 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#include <QPainter>
#include <QPaintEvent>
#include <QStyleOptionMenuItem>

#include <algorithm>

#include <retroshare/rsmsgs.h>

#include "TagsMenu.h"
#include "gui/common/TagDefs.h"
#include "gui/settings/NewTag.h"
#include "gui/notifyqt.h"
#include "util/qtthreadsutils.h"

#include "gui/msgs/MessageInterface.h"

#define ACTION_TAGSINDEX_SIZE  3
#define ACTION_TAGSINDEX_TYPE  "Type"
#define ACTION_TAGSINDEX_ID    "ID"
#define ACTION_TAGSINDEX_COLOR "Color"

#define ACTION_TAGS_REMOVEALL 0
#define ACTION_TAGS_TAG       1
#define ACTION_TAGS_NEWTAG    2

TagsMenu::TagsMenu(const QString &title, QWidget *parent)
	: QMenu (title, parent)
{
	connect(this, SIGNAL(triggered (QAction*)), this, SLOT(tagTriggered(QAction*)));

	mEventHandlerId = 0;
	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }); }, mEventHandlerId, RsEventType::MAIL_TAG );

	fillTags();
}

TagsMenu::~TagsMenu()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId);
}

void TagsMenu::paintEvent(QPaintEvent *e)
{
	QMenu::paintEvent(e);

	QPainter p(this);
	QRegion emptyArea = QRegion(rect());

	//draw the items with color
	foreach (QAction *action, actions()) {
		QRect adjustedActionRect = actionGeometry(action);
		if (!e->rect().intersects(adjustedActionRect))
		   continue;

		const QMap<QString, QVariant> &values = action->data().toMap();
		if (values.size () != ACTION_TAGSINDEX_SIZE) {
			continue;
		}
		if (values [ACTION_TAGSINDEX_TYPE] != ACTION_TAGS_TAG) {
			continue;
		}

		//set the clip region to be extra safe (and adjust for the scrollers)
		QRegion adjustedActionReg(adjustedActionRect);
		emptyArea -= adjustedActionReg;
		p.setClipRegion(adjustedActionReg);

		QStyleOptionMenuItem opt;
		initStyleOption(&opt, action);

		opt.palette.setColor(QPalette::ButtonText, QColor(values [ACTION_TAGSINDEX_COLOR].toInt()));
		// needed for Cleanlooks
		opt.palette.setColor(QPalette::Text, QColor(values [ACTION_TAGSINDEX_COLOR].toInt()));

		opt.rect = adjustedActionRect;
		style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);
	}
}

void TagsMenu::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if (event->mType != RsEventType::MAIL_TAG) {
		return;
	}

	const RsMailTagEvent *fe = dynamic_cast<const RsMailTagEvent*>(event.get());
	if (!fe) {
		return;
	}

	switch (fe->mMailTagEventCode) {
	case RsMailTagEventCode::TAG_ADDED:
	case RsMailTagEventCode::TAG_CHANGED:
	case RsMailTagEventCode::TAG_REMOVED:
		fillTags();
		break;
	}
}

void TagsMenu::fillTags()
{
	clear();

	MsgTagType tags;
	rsMail->getMessageTagTypes(tags);
	std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tag;

	bool user = false;

	QString text;
	QAction *action;
	QMap<QString, QVariant> values;

	if (tags.types.size()) {
		action = new QAction(tr("Remove All Tags"), this);
		values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_REMOVEALL;
		values [ACTION_TAGSINDEX_ID] = 0;
		values [ACTION_TAGSINDEX_COLOR] = 0;
		action->setData (values);
		addAction(action);

		addSeparator();

		for (tag = tags.types.begin(); tag != tags.types.end(); ++tag) {
			text = TagDefs::name(tag->first, tag->second.first);

			action = new QAction(text, this);
			values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_TAG;
			values [ACTION_TAGSINDEX_ID] = tag->first;
			values [ACTION_TAGSINDEX_COLOR] = QRgb(tag->second.second);
			action->setData (values);
			action->setCheckable(true);

			if (tag->first >= RS_MSGTAGTYPE_USER && user == false) {
				user = true;
				addSeparator();
			}

			addAction(action);
		}

		addSeparator();
	}

	action = new QAction(tr("New tag ..."), this);
	values [ACTION_TAGSINDEX_TYPE] = ACTION_TAGS_NEWTAG;
	values [ACTION_TAGSINDEX_ID] = 0;
	values [ACTION_TAGSINDEX_COLOR] = 0;
	action->setData (values);
	addAction(action);
}

void TagsMenu::activateActions(std::set<uint32_t>& tagIds)
{
	foreach(QObject *object, children()) {
		QAction *action = qobject_cast<QAction*> (object);
		if (action == NULL) {
			continue;
		}

		const QMap<QString, QVariant> &values = action->data().toMap();
		if (values.size () != ACTION_TAGSINDEX_SIZE) {
			continue;
		}
		if (values [ACTION_TAGSINDEX_TYPE] != ACTION_TAGS_TAG) {
			continue;
		}

        auto tagId = std::find(tagIds.begin(), tagIds.end(), values [ACTION_TAGSINDEX_ID]);
		action->setChecked(tagId != tagIds.end());
	}
}

void TagsMenu::tagTriggered(QAction *action)
{
	if (action == NULL) {
		return;
	}

	const QMap<QString, QVariant> &values = action->data().toMap();
	if (values.size () != ACTION_TAGSINDEX_SIZE) {
		return;
	}

	if (values [ACTION_TAGSINDEX_TYPE] == ACTION_TAGS_REMOVEALL) {
		// remove all tags
		emit tagRemoveAll();
	} else if (values [ACTION_TAGSINDEX_TYPE] == ACTION_TAGS_NEWTAG) {
		// new tag
		MsgTagType tags;
		rsMail->getMessageTagTypes(tags);

		NewTag tagDlg(tags);
		if (tagDlg.exec() == QDialog::Accepted && tagDlg.m_nId) {
			std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tag = tags.types.find(tagDlg.m_nId);
			if (tag != tags.types.end()) {
				if (rsMail->setMessageTagType(tag->first, tag->second.first, tag->second.second)) {
					emit tagSet(tagDlg.m_nId, true);
				}
			}
		}
	}  else if (values [ACTION_TAGSINDEX_TYPE].toInt() == ACTION_TAGS_TAG) {
		int tagId = values [ACTION_TAGSINDEX_ID].toInt();
		if (tagId) {
			emit tagSet(tagId, action->isChecked());
		}
	}
}
