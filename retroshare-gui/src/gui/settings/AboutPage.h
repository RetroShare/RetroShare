/*******************************************************************************
 * gui/settings/AboutPage.h                                                    *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
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

#pragma once

#include <QFileDialog>

#include <retroshare-gui/configpage.h>
#include "ui_AboutPage.h"

class AboutPage : public ConfigPage
{
	Q_OBJECT

public:
	/** Default Constructor */
	AboutPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	/** Default Destructor */
	~AboutPage();

	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/icons/svg/info.svg") ; }
	virtual QString pageName() const { return tr("About") ; }
	virtual QString helpText() const { return ""; }

private:

	/** Qt Designer generated object */
	Ui::AboutPage ui;
};
