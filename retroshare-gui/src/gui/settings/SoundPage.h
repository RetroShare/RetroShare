/*******************************************************************************
 * gui/settings/SoundPage.h                                                    *
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

#ifndef _SOUNDPAGE_H
#define _SOUNDPAGE_H

#include <QFileDialog>

#include <retroshare-gui/configpage.h>
#include "ui_SoundPage.h"

class SoundPage : public ConfigPage
{
	Q_OBJECT

public:
	/** Default Constructor */
	SoundPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	/** Default Destructor */
	~SoundPage();

	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/sound.svg") ; }
	virtual QString pageName() const { return tr("Sound") ; }
	virtual QString helpText() const { return ""; }

private slots:
	void eventChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
	void filenameChanged(QString filename);
	void defaultButtonClicked();
	void browseButtonClicked();
	void playButtonClicked();
	void updateSounds();

private:
	QTreeWidgetItem *addGroup(const QString &name);
	QTreeWidgetItem *addItem(QTreeWidgetItem *groupItem, const QString &name, const QString &event);

	/** Qt Designer generated object */
	Ui::SoundPage ui;
};

#endif

