/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include "SoundPage.h"
#include "rsharesettings.h"
#include "util/misc.h"

#include <retroshare/rsplugin.h>

#define COLUMN_NAME     0
#define COLUMN_FILENAME 1
#define COLUMN_COUNT    2
#define COLUMN_DATA     COLUMN_NAME

#define ROLE_EVENT      Qt::UserRole

#define TYPE_GROUP      0
#define TYPE_ITEM       1

/** Constructor */
SoundPage::SoundPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	connect(ui.eventTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(eventChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(ui.filenameEdit, SIGNAL(textChanged(QString)), this, SLOT(filenameChanged(QString)));
	connect(ui.browseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(ui.playButton, SIGNAL(clicked()), this, SLOT(playButtonClicked()));

	ui.eventTreeWidget->setColumnCount(COLUMN_COUNT);

	QTreeWidgetItem *headerItem = ui.eventTreeWidget->headerItem();
	headerItem->setText(COLUMN_NAME, tr("Event"));
	headerItem->setText(COLUMN_FILENAME, tr("Filename"));

	ui.eventTreeWidget->header()->setResizeMode(QHeaderView::Fixed);
	ui.eventTreeWidget->setTextElideMode(Qt::ElideMiddle);

	/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

SoundPage::~SoundPage()
{
}

QTreeWidgetItem *SoundPage::addGroup(const QString &name)
{
	QTreeWidgetItem *item = NULL;

	int count = ui.eventTreeWidget->topLevelItemCount();
	for (int i = 0; i < count; ++i) {
		item = ui.eventTreeWidget->topLevelItem(i);
		if (item->text(COLUMN_NAME) == name) {
			return item;
		}
	}

	item = new QTreeWidgetItem(TYPE_GROUP);
	item->setText(COLUMN_NAME, name);
	ui.eventTreeWidget->insertTopLevelItem(ui.eventTreeWidget->topLevelItemCount(), item);
	ui.eventTreeWidget->expandItem(item);

	return item;
}

QTreeWidgetItem *SoundPage::addItem(QTreeWidgetItem *groupItem, const QString &name, const QString &event)
{
	QTreeWidgetItem *item = new QTreeWidgetItem(TYPE_ITEM);
	item->setData(COLUMN_DATA, ROLE_EVENT, event);
	item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
	item->setCheckState(COLUMN_NAME, soundManager->eventEnabled(event) ? Qt::Checked : Qt::Unchecked);
	item->setText(COLUMN_NAME, name);
	item->setText(COLUMN_FILENAME, soundManager->eventFilename(event));
	groupItem->addChild(item);

	return item;
}

/** Saves the changes on this page */
bool SoundPage::save(QString &/*errmsg*/)
{
	QTreeWidgetItemIterator itemIterator(ui.eventTreeWidget);
	QTreeWidgetItem *item = NULL;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		if (item->type() == TYPE_ITEM) {
			const QString event = item->data(COLUMN_DATA, ROLE_EVENT).toString();
			soundManager->setEventEnabled(event, item->checkState(COLUMN_NAME) == Qt::Checked);
			soundManager->setEventFilename(event, item->text(COLUMN_FILENAME));
		}
	}

	return true;
}

/** Loads the settings for this page */
void SoundPage::load()
{
	ui.eventTreeWidget->clear();

	/* add standard events */

	QTreeWidgetItem *groupItem = addGroup(tr("Friend"));
	addItem(groupItem, tr("go Online"), SOUND_USER_ONLINE);

	groupItem = addGroup(tr("Chatmessage"));
	addItem(groupItem, tr("New Msg"), SOUND_NEW_CHAT_MESSAGE);

	groupItem = addGroup(tr("Message"));
	addItem(groupItem, tr("Message arrived"), SOUND_MESSAGE_ARRIVED);

	groupItem = addGroup(tr("Download"));
	addItem(groupItem, tr("Download complete"), SOUND_DOWNLOAD_COMPLETE);

	/* add plugin events */
	int pluginCount = rsPlugins->nbPlugins();
	for (int i = 0; i < pluginCount; ++i) {
		RsPlugin *plugin = rsPlugins->plugin(i);

		if (plugin) {
			SoundEvents events;
			plugin->qt_sound_events(events);

			if (events.mEventInfos.empty()) {
				continue;
			}

			QList<SoundEvents::SoundEventInfo>::iterator it;
			for (it = events.mEventInfos.begin(); it != events.mEventInfos.end(); ++it) {
				groupItem = addGroup(it->mGroupName);
				addItem(groupItem, it->mEventName, it->mEvent);
			}
		}
	}

	ui.eventTreeWidget->resizeColumnToContents(COLUMN_NAME);

	eventChanged(NULL, NULL);
}

void SoundPage::eventChanged(QTreeWidgetItem *current, QTreeWidgetItem */*previous*/)
{
	if (!current || current->type() != TYPE_ITEM) {
		ui.eventGroup->setEnabled(false);
		ui.eventName->clear();
		ui.filenameEdit->clear();
		ui.playButton->setEnabled(false);
		return;
	}

	ui.eventGroup->setEnabled(true);
	ui.filenameEdit->setText(current->text(COLUMN_FILENAME));

	QString eventName;
	if (current->parent()) {
		eventName = current->parent()->text(COLUMN_NAME) + ": ";
	}
	eventName += current->text(COLUMN_NAME);
	ui.eventName->setText(eventName);
}

void SoundPage::filenameChanged(QString filename)
{
	ui.playButton->setEnabled(!filename.isEmpty());

	QTreeWidgetItem *item = ui.eventTreeWidget->currentItem();
	if (item) {
		item->setText(COLUMN_FILENAME, filename);
	}
}

void SoundPage::browseButtonClicked()
{
	QString filename;
	if (!misc::getOpenFileName(this, RshareSettings::LASTDIR_SOUNDS, tr("Open File"), "wav (*.wav)", filename)) {
		return;
	}
	ui.filenameEdit->setText(filename);
}

void SoundPage::playButtonClicked()
{
	QTreeWidgetItem *item = ui.eventTreeWidget->currentItem();
	if (!item) {
		return;
	}

	QString filename = item->text(COLUMN_FILENAME);
	soundManager->playFile(filename);
}
