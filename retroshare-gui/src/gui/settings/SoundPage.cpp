/*******************************************************************************
 * gui/settings/SoundPage.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2009 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "SoundPage.h"
#include "rsharesettings.h"
#include "util/misc.h"
#include "gui/SoundManager.h"

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
	connect(ui.defaultButton, SIGNAL(clicked()), this, SLOT(defaultButtonClicked()));
	connect(ui.browseButton, SIGNAL(clicked()), this, SLOT(browseButtonClicked()));
	connect(ui.playButton, SIGNAL(clicked()), this, SLOT(playButtonClicked()));
	connect(ui.eventTreeWidget, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(updateSounds()));

	ui.eventTreeWidget->setColumnCount(COLUMN_COUNT);

	QTreeWidgetItem *headerItem = ui.eventTreeWidget->headerItem();
	headerItem->setText(COLUMN_NAME, tr("Event"));
	headerItem->setText(COLUMN_FILENAME, tr("Filename"));

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
	ui.eventTreeWidget->header()->setSectionResizeMode(QHeaderView::Fixed);
#else
	ui.eventTreeWidget->header()->setResizeMode(QHeaderView::Fixed);
#endif
	ui.eventTreeWidget->setTextElideMode(Qt::ElideMiddle);

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
	item->setCheckState(COLUMN_NAME, SoundManager::eventEnabled(event) ? Qt::Checked : Qt::Unchecked);
	item->setText(COLUMN_NAME, name);
	item->setText(COLUMN_FILENAME, SoundManager::eventFilename(event));
	groupItem->addChild(item);

	return item;
}

/** Saves the changes on this page */
void SoundPage::updateSounds()
{
	QTreeWidgetItemIterator itemIterator(ui.eventTreeWidget);
	QTreeWidgetItem *item = NULL;

	while ((item = *itemIterator) != NULL)
    {
		++itemIterator;

		if (item->type() == TYPE_ITEM)
        {
			const QString event = item->data(COLUMN_DATA, ROLE_EVENT).toString();

            std::cerr << "Enabling event \"" << event.toStdString() << "\" to " << item->checkState(COLUMN_NAME) << ", to file \"" << item->text(COLUMN_FILENAME).toStdString() << std::endl;
			SoundManager::setEventEnabled(event, item->checkState(COLUMN_NAME) == Qt::Checked);
			SoundManager::setEventFilename(event, item->text(COLUMN_FILENAME));
		}
	}
}

/** Loads the settings for this page */
void SoundPage::load()
{
	SignalsBlocker<QTreeWidget> B(ui.eventTreeWidget) ;

    ui.eventTreeWidget->clear();

	/* add sound events */
	SoundEvents events;
	SoundManager::soundEvents(events);

	QString event;
	foreach (event, events.mEventInfos.keys()) {
		SoundEvents::SoundEventInfo &eventInfo = events.mEventInfos[event];

		QTreeWidgetItem *groupItem = addGroup(eventInfo.mGroupName);
		addItem(groupItem, eventInfo.mEventName, event);
	}

	ui.eventTreeWidget->resizeColumnToContents(COLUMN_NAME);
	ui.eventTreeWidget->sortByColumn(COLUMN_NAME, Qt::AscendingOrder);

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

	QString event = current->data(COLUMN_DATA, ROLE_EVENT).toString();
	whileBlocking(ui.defaultButton)->setDisabled(SoundManager::defaultFilename(event, true).isEmpty());
}

void SoundPage::filenameChanged(QString filename)
{
	ui.playButton->setEnabled(!filename.isEmpty());

	QTreeWidgetItem *item = ui.eventTreeWidget->currentItem();
	if (item) {
		item->setText(COLUMN_FILENAME, filename);
	}
}

void SoundPage::defaultButtonClicked()
{
	QTreeWidgetItem *item = ui.eventTreeWidget->currentItem();
	if (!item) {
		return;
	}

	QString event = item->data(COLUMN_DATA, ROLE_EVENT).toString();
	ui.filenameEdit->setText(SoundManager::defaultFilename(event, true));
}

void SoundPage::browseButtonClicked()
{
	QString filename;
	if (!misc::getOpenFileName(this, RshareSettings::LASTDIR_SOUNDS, tr("Open File"), "wav (*.wav)", filename)) {
		return;
	}
	ui.filenameEdit->setText(SoundManager::convertFilename(filename));
}

void SoundPage::playButtonClicked()
{
	QTreeWidgetItem *item = ui.eventTreeWidget->currentItem();
	if (!item) {
		return;
	}

	QString filename = item->text(COLUMN_FILENAME);
	SoundManager::playFile(filename);
}
