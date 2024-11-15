/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdDetailsDialog.h                           *
 *                                                                             *
 *  Copyright (C) 2014 - 2010 RetroShare Team <retroshare.project@gmail.com>   *
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

#ifndef _IDDETAILSDIALOG_H
#define _IDDETAILSDIALOG_H

#include <QDialog>

#include <retroshare/rsidentity.h>

namespace Ui {
class IdDetailsDialog;
}

class UIStateHelper;

class IdDetailsDialog : public QDialog
{
	Q_OBJECT

public:
	/** Default constructor */
	IdDetailsDialog(const RsGxsGroupId &id, QWidget *parent = 0);
	/** Default destructor */
	~IdDetailsDialog();

private slots:
	void modifyReputation();
	void toggleAutoBanIdentities(bool b);
	
	static QString inviteMessage();
	void sendInvite();
private :
	void loadIdentity();
	void loadIdentity(RsGxsIdGroup data);

private:
	RsGxsGroupId mId;
	UIStateHelper *mStateHelper;

	/** Qt Designer generated object */
	Ui::IdDetailsDialog *ui;
};

#endif
