/*******************************************************************************
 * gui/settings/JsonApiPage.h                                                  *
 *                                                                             *
 * Copyright (C) 2018-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include "retroshare-gui/configpage.h"
#include "gui/common/FilesDefs.h"
#include "ui_JsonApiPage.h"

class JsonApiPage : public ConfigPage
{
	Q_OBJECT

public:

	JsonApiPage(QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
	~JsonApiPage() override = default;

	virtual QPixmap iconPixmap() const override
	{
		return FilesDefs::getPixmapFromQtResourcePath(
		            ":/icons/svg/empty-circle.svg" );
	}

	virtual QString pageName() const override { return tr("JSON API"); }
	virtual QString helpText() const override;

	/** Call this after start of libretroshare/Retroshare
	 *  checks the settings and starts JSON API if required */
	static bool checkStartJsonApi();

	/** call this before shutdown of libretroshare
	 *  it stops the JSON API if its running */
	static void checkShutdownJsonApi();

public slots:
	void load() override;

	void onApplyClicked();
	void addTokenClicked();
	void removeTokenClicked();
	void tokenClicked(const QModelIndex& index);
	void enableJsonApi(bool checked);
	bool updateParams();
	void checkToken(QString);

private:
	Ui::JsonApiPage ui; /// Qt Designer generated object
};
